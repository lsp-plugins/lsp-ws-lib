/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 15 янв. 2025 г.
 *
 * lsp-ws-lib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-ws-lib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-ws-lib. If not, see <https://www.gnu.org/licenses/>.
 */

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <private/freetype/FontManager.h>
#include <private/gl/Batch.h>
#include <private/gl/Gradient.h>
#include <private/gl/Stats.h>
#include <private/gl/Surface.h>
#include <private/x11/X11Display.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            #define ADD_TVERTEX(v, v_ci, v_x, v_y, v_s, v_t) \
                v->x        = v_x; \
                v->y        = v_y; \
                v->s        = v_s; \
                v->t        = v_t; \
                v->cmd      = v_ci; \
                ++v;

            #define ADD_VERTEX(v, v_ci, v_x, v_y)  ADD_TVERTEX(v, v_ci, v_x, v_y, 0.0f, 0.0f)

            #define ADD_HRECTANGLE(v, a, b, c, d) \
                v[0]        = a; \
                v[1]        = b; \
                v[2]        = c; \
                v[3]        = a; \
                v[4]        = c; \
                v[5]        = d; \
                v += 6;

            #define ADD_HTRIANGLE(v, a, b, c) \
                v[0]        = a; \
                v[1]        = b; \
                v[2]        = c; \
                v += 3;

            constexpr float k_color = 1.0f / 255.0f;

            Surface::Surface(IDisplay *display, gl::IContext *ctx, size_t width, size_t height):
                ISurface(width, height, ST_OPENGL),
                sBatch(ctx->allocator())
            {
                OPENGL_INC_STATS(surface_alloc);

                pDisplay        = display;
                pContext        = safe_acquire(ctx);
                pTexture        = NULL;
                pText           = new TextAllocator(pContext);
                nWidth          = width;
                nHeight         = height;
                nNumClips       = 0;
                bNested         = false;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);
                sOrigin.left    = 0;
                sOrigin.top     = 0;

                sBatch.init();
                sync_matrix();

                lsp_trace("primary surface created ptr=%p", this);
            }

            Surface::Surface(gl::IContext *ctx, gl::TextAllocator *text, size_t width, size_t height):
                ISurface(width, height, ST_OPENGL),
                sBatch(ctx->allocator())
            {
                OPENGL_INC_STATS(surface_alloc);

                pDisplay        = NULL;
                pContext        = safe_acquire(ctx);
                pTexture        = NULL;
                pText           = safe_acquire(text);
                nWidth          = width;
                nHeight         = height;
                bNested         = true;
                nNumClips       = 0;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);
                sOrigin.left    = 0;
                sOrigin.top     = 0;

                sBatch.init();
                sync_matrix();
            }

            Surface *Surface::create_nested(gl::TextAllocator *text, size_t width, size_t height)
            {
                return new Surface(pContext, text, width, height);
            }

            IDisplay *Surface::display()
            {
                return pDisplay;
            }

            ISurface *Surface::create(size_t width, size_t height)
            {
                Surface *s = create_nested(pText, width, height);
                if (s != NULL)
                    s->pDisplay     = pDisplay;

                return s;
            }

            IGradient *Surface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                return new gl::Gradient(
                    gl::Gradient::linear_t {
                        x0, y0,
                        x1, y1});
            }

            IGradient *Surface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
            {
                return new gl::Gradient(
                    gl::Gradient::radial_t {
                        cx0, cy0,
                        cx1, cy1,
                        r});
            }

            void Surface::do_destroy()
            {
                sBatch.clear();

                if ((pContext != NULL) && (!bNested))
                {
                    pContext->invalidate();
                    lsp_trace("primary surface destroyed ptr=%p", this);
                }

                safe_release(pTexture);
                safe_release(pText);
                safe_release(pContext);

                pDisplay        = NULL;
                pContext        = NULL;
            }

            Surface::~Surface()
            {
                OPENGL_INC_STATS(surface_free);

                do_destroy();

                OPENGL_OUTPUT_STATS(true);
            }

            void Surface::destroy()
            {
                do_destroy();
            }

            inline ssize_t Surface::make_command(ssize_t index, cmd_color_t color) const
            {
                return (index << 5) | (size_t(color) << 3) | nNumClips;
            }

            float *Surface::serialize_clipping(float *dst) const
            {
                for (size_t i=0; i<nNumClips; ++i)
                {
                    const clip_rect_t *r = &vClips[i];
                    dst[0]          = r->left;
                    dst[1]          = r->top;
                    dst[2]          = r->right;
                    dst[3]          = r->bottom;

                    dst            += 4;
                }

                return dst;
            }

            inline float *Surface::serialize_color(float *dst, float r, float g, float b, float a)
            {
                a               = 1.0f - a;
                dst[0]          = r * a;
                dst[1]          = g * a;
                dst[2]          = b * a;
                dst[3]          = a;

                return dst + 4;
            }

            inline float *Surface::serialize_color(float *dst, const Color & c)
            {
                const float a   = 1.0f - c.alpha();
                dst[0]          = c.red() * a;
                dst[1]          = c.green() * a;
                dst[2]          = c.blue() * a;
                dst[3]          = a;

                return dst + 4;
            }

            inline float *Surface::serialize_texture(float *dst, const gl::Texture *t)
            {
                dst[0]          = float(t->width());
                dst[1]          = float(t->height());
                dst[2]          = t->format();
                dst[3]          = t->multisampling();

                return dst + 4;
            }

            void Surface::sync_matrix()
            {
                // Set-up drawing matrix
                const float dx = 2.0f / float(nWidth);
                const float dy = 2.0f / float(nHeight);

                vMatrix[0]  = dx;
                vMatrix[1]  = 0.0f;
                vMatrix[2]  = 0.0f;
                vMatrix[3]  = 0.0f;

                vMatrix[4]  = 0.0f;
                vMatrix[5]  = -dy;
                vMatrix[6]  = 0.0f;
                vMatrix[7]  = 0.0f;

                vMatrix[8]  = 0.0f;
                vMatrix[9]  = 0.0f;
                vMatrix[10] = 1.0f;
                vMatrix[11] = 0.0f;

                vMatrix[12] = -1.0f;
                vMatrix[13] = 1.0f;
                vMatrix[14] = 0.0f;
                vMatrix[15] = 1.0f;
            }

            uint32_t Surface::enrich_flags(uint32_t flags) const
            {
                if (bAntiAliasing)
                    flags      |= BATCH_MULTISAMPLE;
                return flags;
            }

            inline void Surface::extend_rect(clip_rect_t &rect, float x, float y)
            {
                rect.left           = lsp_min(rect.left, x);
                rect.top            = lsp_min(rect.top, y);
                rect.right          = lsp_max(rect.right, x);
                rect.bottom         = lsp_max(rect.bottom, y);
            }

            inline void Surface::limit_rect(clip_rect_t & rect)
            {
                rect.left           = lsp_max(rect.left, -sOrigin.left);
                rect.top            = lsp_max(rect.top, -sOrigin.top);
                rect.right          = lsp_min(rect.right, float(nWidth) - sOrigin.left);
                rect.bottom         = lsp_min(rect.bottom, float(nHeight) - sOrigin.top);
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, float r, float g, float b, float a)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        sOrigin.left,
                        sOrigin.top,
                        enrich_flags(flags),
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + nNumClips * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf);
                serialize_color(buf, r, g, b, a);

                return make_command(index, C_SOLID);
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, const Color & color)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        sOrigin.left,
                        sOrigin.top,
                        enrich_flags(flags),
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + nNumClips * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf);
                serialize_color(buf, color);

                return make_command(index, C_SOLID);
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, const IGradient * g)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;
                if (g == NULL)
                    return -STATUS_BAD_ARGUMENTS;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        sOrigin.left,
                        sOrigin.top,
                        enrich_flags(flags),
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                const gl::Gradient *grad = static_cast<const gl::Gradient *>(g);
                const size_t szof = grad->serial_size();

                // Allocate place for command
                float *buf = NULL;
                ssize_t index = sBatch.command(&buf, (szof + nNumClips * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf);
                grad->serialize(buf);

                return make_command(index, grad->linear() ? C_LINEAR : C_RADIAL);
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, gl::Texture *t, float a)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;
                if (t == NULL)
                    return -STATUS_BAD_ARGUMENTS;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        sOrigin.left,
                        sOrigin.top,
                        enrich_flags(flags),
                        t,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + nNumClips * sizeof(clip_rect_t) + 4 * sizeof(float)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf);
                buf     = serialize_color(buf, 1.0f, 1.0f, 1.0f, a);
                serialize_texture(buf, t);

                return make_command(index, C_TEXTURE);
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, gl::Texture *t, const Color & color)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;
                if (t == NULL)
                    return -STATUS_BAD_ARGUMENTS;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        sOrigin.left,
                        sOrigin.top,
                        enrich_flags(flags),
                        t,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + nNumClips * sizeof(clip_rect_t) + 4 * sizeof(float)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf);
                buf     = serialize_color(buf, color);
                serialize_texture(buf, t);

                return make_command(index, C_TEXTURE);
            }

            gl::Texture *Surface::make_text(texture_rect_t *rect, const void *data, size_t width, size_t height, size_t stride)
            {
                // Check that texture can be placed to the atlas
                gl::Texture *tex = NULL;
                if ((pText != NULL) && (width <= TEXT_ATLAS_SIZE) && (height <= TEXT_ATLAS_SIZE))
                {
                    ws::rectangle_t wrect;

                    tex = pText->allocate(&wrect, data, width, height, stride);
                    if (tex != NULL)
                    {
                        rect->sb        = wrect.nLeft * gl::TEXT_ATLAS_SCALE;
                        rect->tb        = wrect.nTop * gl::TEXT_ATLAS_SCALE;
                        rect->se        = (wrect.nLeft + wrect.nWidth) * gl::TEXT_ATLAS_SCALE;
                        rect->te        = (wrect.nTop + wrect.nHeight) * gl::TEXT_ATLAS_SCALE;
                    }

                    return tex;
                }

                // Allocate texture
                tex = new gl::Texture(pContext);
                if (tex == NULL)
                    return NULL;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                if (tex->set_image(data, width, height, stride, gl::TEXTURE_ALPHA8) != STATUS_OK)
                    return NULL;

                rect->sb        = 0.0f;
                rect->tb        = 0.0f;
                rect->se        = 1.0f;
                rect->te        = 1.0f;

                return release_ptr(tex);
            }

            void Surface::fill_triangle(uint32_t ci, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(3);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0, y0);
                ADD_VERTEX(v, ci, x1, y1);
                ADD_VERTEX(v, ci, x2, y2);

                sBatch.htriangle(vi, vi + 1, vi + 2);
            }

            void Surface::fill_rect(uint32_t ci, float x0, float y0, float x1, float y1)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0, y0);
                ADD_VERTEX(v, ci, x0, y1);
                ADD_VERTEX(v, ci, x1, y1);
                ADD_VERTEX(v, ci, x1, y0);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::fill_textured_rect(uint32_t ci, const texcoord_t & tex, float x0, float y0, float x1, float y1)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                const float tx0     = (x0 - tex.x) * tex.sx;
                const float tx1     = (x1 - tex.x) * tex.sx;
                const float ty0     = (y0 - tex.y) * tex.sy;
                const float ty1     = (y1 - tex.y) * tex.sy;

                ADD_TVERTEX(v, ci, x0, y0, tx0, ty0);
                ADD_TVERTEX(v, ci, x0, y1, tx0, ty1);
                ADD_TVERTEX(v, ci, x1, y1, tx1, ty1);
                ADD_TVERTEX(v, ci, x1, y0, tx1, ty0);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::draw_line(uint32_t ci, float x0, float y0, float x1, float y1, float width)
            {
                // Find first not short segment
                width          *= 0.5f;
                const float dx  = x1 - x0;
                const float dy  = y1 - y0;
                const float d   = dx*dx + dy*dy;
                if (d <= 1e-10f)
                    return;

                // Draw first segment
                const float kd  = width / sqrtf(d);
                const float ndx = -dy * kd;
                const float ndy = dx * kd;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0 + ndx, y0 + ndy);
                ADD_VERTEX(v, ci, x0 - ndx, y0 - ndy);
                ADD_VERTEX(v, ci, x1 - ndx, y1 - ndy);
                ADD_VERTEX(v, ci, x1 + ndx, y1 + ndy);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::fill_triangle_fan(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, size_t n)
            {
                if (n < 3)
                    return;

                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(n);
                if (v == NULL)
                    return;
                vertex_t * const v_end  = &v[n];
                lsp_finally {
                    if (v < v_end)
                        sBatch.release_vertices(v_end - v);
                };

                void *iv_raw    = sBatch.add_indices((n-2) * 3, v0i + n - 1);
                if (iv_raw == NULL)
                    return;

                uint32_t vi         = v0i + 1;
                ADD_VERTEX(v, ci, x[0], y[0]);
                ADD_VERTEX(v, ci, x[1], y[1]);

                rect.left           = lsp_min(x[0], x[1]);
                rect.top            = lsp_min(y[0], y[1]);
                rect.right          = lsp_max(x[0], x[1]);
                rect.bottom         = lsp_max(y[0], y[1]);

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv        = static_cast<uint16_t *>(iv_raw);
                        for (size_t i=2; i<n; ++i)
                        {
                            extend_rect(rect, x[i], y[i]);
                            ADD_VERTEX(v, ci, x[i], y[i]);
                            ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(vi), uint16_t(vi + 1));
                            ++vi;
                        }
                        break;
                    }

                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv        = static_cast<uint32_t *>(iv_raw);
                        for (size_t i=2; i<n; ++i)
                        {
                            extend_rect(rect, x[i], y[i]);
                            ADD_VERTEX(v, ci, x[i], y[i]);
                            ADD_HTRIANGLE(iv, v0i, vi, vi + 1);
                            ++vi;
                        }
                        break;
                    }

                    case INDEX_FMT_U8:
                    default:
                    {
                        uint8_t *iv         = static_cast<uint8_t *>(iv_raw);
                        for (size_t i=2; i<n; ++i)
                        {
                            extend_rect(rect, x[i], y[i]);
                            ADD_VERTEX(v, ci, x[i], y[i]);
                            ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(vi), uint8_t(vi + 1));
                            ++vi;
                        }

                        break;
                    }
                }

                limit_rect(rect);
            }

            void Surface::fill_circle(uint32_t ci, float x, float y, float r)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float phi     = lsp_min(M_PI / r, M_PI_4);
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const size_t count  = M_PI * 2.0f / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx = r;
                float vy = 0.0f;

                ADD_VERTEX(v, ci, x, y);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                for (size_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    ADD_VERTEX(v, ci, x + vx, y + vy);
                }

                ADD_VERTEX(v, ci, x + r, y);

                // Generate indices
                sBatch.htriangle_fan(v0i, count + 1);
            }

            void Surface::fill_sector(uint32_t ci, float x, float y, float r, float a1, float a2)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float phi     = (delta > 0.0f) ? lsp_min(M_PI / r, M_PI_4) : lsp_min(-M_PI / r, M_PI_4);
                const float ex      = cosf(a2) * r;
                const float ey      = sinf(a2) * r;
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const size_t n      = count + 3;
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;
                vertex_t * const v_end  = &v[n];
                lsp_finally {
                    if (v < v_end)
                        sBatch.release_vertices(v_end - v);
                };

                void *iv_raw    = sBatch.add_indices((count + 1) * 3, v0i + n - 1);
                if (iv_raw == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * r;
                float vy            = sinf(a1) * r;

                uint32_t v1i        = v0i + 1;
                ADD_VERTEX(v, ci, x, y);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv        = static_cast<uint16_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                        break;
                    }

                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv        = static_cast<uint32_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);
                        break;
                    }

                    case INDEX_FMT_U8:
                    default:
                    {
                        uint8_t *iv         = static_cast<uint8_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                        break;
                    }
                }
            }

            void Surface::fill_textured_sector(uint32_t ci, const texcoord_t & tex, float x, float y, float r, float a1, float a2)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float phi     = (delta > 0.0f) ? lsp_min(M_PI / r, M_PI_4) : lsp_min(-M_PI / r, M_PI_4);
                const float ex      = cosf(a2) * r;
                const float ey      = sinf(a2) * r;
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const size_t n      = count + 3;
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;
                vertex_t * const v_end  = &v[n];
                lsp_finally {
                    if (v < v_end)
                        sBatch.release_vertices(v_end - v);
                };

                void *iv_raw    = sBatch.add_indices((count + 1) * 3, v0i + n - 1);
                if (iv_raw == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * r;
                float vy            = sinf(a1) * r;

                uint32_t v1i        = v0i + 1;

                ADD_TVERTEX(v, ci, x, y, (x - tex.x) * tex.sx, (y - tex.y) * tex.sy);
                float xx            = x + vx;
                float yy            = y + vy;
                float txx           = (xx - tex.x) * tex.sx;
                float tyy           = (yy - tex.y) * tex.sy;
                ADD_TVERTEX(v, ci, xx, yy, txx, tyy);

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv        = static_cast<uint16_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            xx          = x + vx;
                            yy          = y + vy;
                            txx         = (xx - tex.x) * tex.sx;
                            tyy         = (yy - tex.y) * tex.sy;
                            ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                            ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                            ++v1i;
                        }

                        xx          = x + ex;
                        yy          = y + ey;
                        txx         = (xx - tex.x) * tex.sx;
                        tyy         = (yy - tex.y) * tex.sy;
                        ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                        ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                        break;
                    }

                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv        = static_cast<uint32_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            xx          = x + vx;
                            yy          = y + vy;
                            txx         = (xx - tex.x) * tex.sx;
                            tyy         = (yy - tex.y) * tex.sy;
                            ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                            ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);
                            ++v1i;
                        }

                        xx          = x + ex;
                        yy          = y + ey;
                        txx         = (xx - tex.x) * tex.sx;
                        tyy         = (yy - tex.y) * tex.sy;
                        ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                        ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);
                        break;
                    }

                    case INDEX_FMT_U8:
                    default:
                    {
                        uint8_t *iv         = static_cast<uint8_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            xx          = x + vx;
                            yy          = y + vy;
                            txx         = (xx - tex.x) * tex.sx;
                            tyy         = (yy - tex.y) * tex.sy;
                            ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                            ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                            ++v1i;
                        }

                        xx          = x + ex;
                        yy          = y + ey;
                        txx         = (xx - tex.x) * tex.sx;
                        tyy         = (yy - tex.y) * tex.sy;
                        ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                        ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                        break;
                    }
                }
            }

            void Surface::fill_corner(uint32_t ci, float x, float y, float xd, float yd, float r, float a)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;

                const float delta   = M_PI * 0.5f;
                const float phi     = (delta > 0.0f) ? lsp_min(M_PI / r, M_PI_4) : lsp_min(-M_PI / r, M_PI_4);
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const size_t n      = count + 3;
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;
                vertex_t * const v_end  = &v[n];
                lsp_finally {
                    if (v < v_end)
                        sBatch.release_vertices(v_end - v);
                };

                void *iv_raw    = sBatch.add_indices((count + 1) * 3, v0i + n - 1);
                if (iv_raw == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a) * r;
                float vy            = sinf(a) * r;
                const float ex      = -vy;
                const float ey      = vx;

                uint32_t v1i        = v0i + 1;
                ADD_VERTEX(v, ci, xd, yd);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv        = static_cast<uint16_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, uint16_t(v0i), uint16_t(v1i), uint16_t(v1i + 1));
                        break;
                    }

                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv        = static_cast<uint32_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, v0i, v1i, v1i + 1);

                        break;
                    }

                    case INDEX_FMT_U8:
                    default:
                    {
                        uint8_t *iv         = static_cast<uint8_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                            ++v1i;
                        }

                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HTRIANGLE(iv, uint8_t(v0i), uint8_t(v1i), uint8_t(v1i + 1));
                        break;
                    }
                }
            }

            void Surface::wire_arc(uint32_t ci, float x, float y, float r, float a1, float a2, float width)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float hw      = width * 0.5f;
                const float ro      = r + hw;
                const float kr      = lsp_max(r - hw, 0.0f) / ro;

                const float phi     = (delta > 0.0f) ? lsp_min(M_PI / ro, M_PI_4) : lsp_min(-M_PI / ro, M_PI_4);
                const float ex      = cosf(a2) * ro;
                const float ey      = sinf(a2) * ro;

                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const size_t n      = count*2 + 4;
                uint32_t v0i        = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(n);
                if (v == NULL)
                    return;
                vertex_t * const v_end  = &v[n];
                lsp_finally {
                    if (v < v_end)
                        sBatch.release_vertices(v_end - v);
                };

                void *iv_raw    = sBatch.add_indices((count + 1) * 6, v0i + n - 1);
                if (iv_raw == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * ro;
                float vy            = sinf(a1) * ro;

                ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv        = static_cast<uint16_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HRECTANGLE(iv, uint16_t(v0i + 2), uint16_t(v0i), uint16_t(v0i + 1), uint16_t(v0i + 3));
                            v0i        += 2;
                        }

                        ADD_VERTEX(v, ci, x + ex * kr, y + ey * kr);
                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HRECTANGLE(iv, uint16_t(v0i + 2), uint16_t(v0i), uint16_t(v0i + 1), uint16_t(v0i + 3));
                        break;
                    }

                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv        = static_cast<uint32_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HRECTANGLE(iv, v0i + 2, v0i, v0i + 1, v0i + 3);
                            v0i        += 2;
                        }

                        ADD_VERTEX(v, ci, x + ex * kr, y + ey * kr);
                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HRECTANGLE(iv, v0i + 2, v0i, v0i + 1, v0i + 3);

                        break;
                    }

                    case INDEX_FMT_U8:
                    default:
                    {
                        uint8_t *iv         = static_cast<uint8_t *>(iv_raw);
                        for (ssize_t i=0; i<count; ++i)
                        {
                            float nvx   = vx*dx - vy*dy;
                            float nvy   = vx*dy + vy*dx;
                            vx          = nvx;
                            vy          = nvy;

                            ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                            ADD_VERTEX(v, ci, x + vx, y + vy);
                            ADD_HRECTANGLE(iv, uint8_t(v0i + 2), uint8_t(v0i), uint8_t(v0i + 1), uint8_t(v0i + 3));
                            v0i        += 2;
                        }

                        ADD_VERTEX(v, ci, x + ex * kr, y + ey * kr);
                        ADD_VERTEX(v, ci, x + ex, y + ey);
                        ADD_HRECTANGLE(iv, uint8_t(v0i + 2), uint8_t(v0i), uint8_t(v0i + 1), uint8_t(v0i + 3));
                        break;
                    }
                }
            }

            void Surface::fill_rect(uint32_t ci, size_t mask, float radius, float left, float top, float width, float height)
            {
                float right     = left + width;
                float bottom    = top + height;

                if (mask & SURFMASK_T_CORNER)
                {
                    float l         = left;
                    float r         = right;
                    top            += radius;

                    if (mask & SURFMASK_LT_CORNER)
                    {
                        l              += radius;
                        fill_sector(ci, l, top, radius, M_PI, M_PI * 1.5f);
                    }
                    if (mask & SURFMASK_RT_CORNER)
                    {
                        r              -= radius;
                        fill_sector(ci, r, top, radius, M_PI * 1.5f, M_PI * 2.0f);
                    }
                    fill_rect(ci, l, top - radius, r, top);
                }
                if (mask & SURFMASK_B_CORNER)
                {
                    float l         = left;
                    float r         = right;
                    bottom         -= radius;

                    if (mask & SURFMASK_LB_CORNER)
                    {
                        l              += radius;
                        fill_sector(ci, l, bottom, radius, M_PI * 0.5f, M_PI);
                    }
                    if (mask & SURFMASK_RB_CORNER)
                    {
                        r              -= radius;
                        fill_sector(ci, r, bottom, radius, 0.0f, M_PI * 0.5f);
                    }
                    fill_rect(ci, l, bottom, r, bottom + radius);
                }

                fill_rect(ci, left, top, right, bottom);
            }

            void Surface::fill_textured_rect(uint32_t ci, const texcoord_t & tex, size_t mask, float radius, float left, float top, float width, float height)
            {
                float right     = left + width;
                float bottom    = top + height;

                if (mask & SURFMASK_T_CORNER)
                {
                    float l         = left;
                    float r         = right;
                    top            += radius;

                    if (mask & SURFMASK_LT_CORNER)
                    {
                        l              += radius;
                        fill_textured_sector(ci, tex, l, top, radius, M_PI, M_PI * 1.5f);
                    }
                    if (mask & SURFMASK_RT_CORNER)
                    {
                        r              -= radius;
                        fill_textured_sector(ci, tex, r, top, radius, M_PI * 1.5f, M_PI * 2.0f);
                    }
                    fill_textured_rect(ci, tex, l, top - radius, r, top);
                }
                if (mask & SURFMASK_B_CORNER)
                {
                    float l         = left;
                    float r         = right;
                    bottom         -= radius;

                    if (mask & SURFMASK_LB_CORNER)
                    {
                        l              += radius;
                        fill_textured_sector(ci, tex, l, bottom, radius, M_PI * 0.5f, M_PI);
                    }
                    if (mask & SURFMASK_RB_CORNER)
                    {
                        r              -= radius;
                        fill_textured_sector(ci, tex, r, bottom, radius, 0.0f, M_PI * 0.5f);
                    }
                    fill_textured_rect(ci, tex, l, bottom, r, bottom + radius);
                }

                fill_textured_rect(ci, tex, left, top, right, bottom);
            }

            void Surface::wire_rect(uint32_t ci, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                const float xr      = radius - line_width * 0.5f;
                const float right   = left + width;
                const float bottom  = top + height;

                // Bounds of horizontal and vertical rectangles
                float top_l         = left;
                float top_r         = right;
                float bot_l         = top_l;
                float bot_r         = top_r;
                float lef_t         = top + line_width;
                float lef_b         = bottom - line_width;
                float rig_t         = lef_t;
                float rig_b         = lef_b;

                if (mask & SURFMASK_LT_CORNER)
                {
                    top_l           = left + radius;
                    lef_t           = top + radius;
                    wire_arc(ci, top_l, lef_t, xr, M_PI, M_PI * 1.5f, line_width);
                }
                if (mask & SURFMASK_RT_CORNER)
                {
                    top_r           = right - radius;
                    rig_t           = top + radius;
                    wire_arc(ci, top_r, rig_t, xr, M_PI * 1.5f, M_PI * 2.0f, line_width);
                }
                if (mask & SURFMASK_LB_CORNER)
                {
                    bot_l           = left + radius;
                    lef_b           = bottom - radius;
                    wire_arc(ci, bot_l, lef_b, xr, M_PI * 0.5f, M_PI, line_width);
                }
                if (mask & SURFMASK_RB_CORNER)
                {
                    bot_r           = right - radius;
                    rig_b           = bottom - radius;
                    wire_arc(ci, bot_r, rig_b, xr, 0.0f, M_PI * 0.5f, line_width);
                }

                fill_rect(ci, top_l, top, top_r, top + line_width);
                fill_rect(ci, bot_l, bottom - line_width, bot_r, bottom);
                fill_rect(ci, left, lef_t, left + line_width, lef_b);
                fill_rect(ci, right - line_width, rig_t, right, rig_b);
            }

            void Surface::fill_frame(
                uint32_t ci,
                size_t flags, float r,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                const float fxe = fx + fw;
                const float fye = fy + fh;
                const float ixe = ix + iw;
                const float iye = iy + ih;

                // Simple case
                if ((ix >= fxe) || (ixe < fx) || (iy >= fye) || (iye < fy))
                {
                    fill_rect(ci, fx, fy, fxe, fye);
                    return;
                }
                else if ((ix <= fx) && (ixe >= fxe) && (iy <= fy) && (iye >= fye))
                    return;

                if (fy < iy) // Top rectangle
                    fill_rect(ci, fx, fy, fxe, iy);
                if (fye > iye) // Bottom rectangle
                    fill_rect(ci, fx, iye, fxe, fye);

                const float vt  = lsp_max(fy, iy);
                const float vb  = lsp_min(fye, iye);
                if (fx < ix) // Left rectangle
                    fill_rect(ci, fx, vt, ix, vb);
                if (fxe > ixe) // Right rectangle
                    fill_rect(ci, ixe, vt, fxe, vb);

                if (flags & SURFMASK_LT_CORNER)
                    fill_corner(ci, ix + r, iy + r, ix, iy, r, M_PI);
                if (flags & SURFMASK_RT_CORNER)
                    fill_corner(ci, ixe - r, iy + r, ixe, iy, r, 1.5f *M_PI);
                if (flags & SURFMASK_LB_CORNER)
                    fill_corner(ci, ix + r, iye - r, ix, iye, r, 0.5f * M_PI);
                if (flags & SURFMASK_RB_CORNER)
                    fill_corner(ci, ixe - r, iye - r, ixe, iye, r, 0.0f);
            }

            template <class T>
            inline void Surface::draw_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, const float *x, const float *y, float width, size_t n)
            {
                vertex_t *v     = vertices;
                T *iv           = indices;
                lsp_finally
                {
                    vertices        = v;
                    indices         = iv;
                };

                size_t i;
                float dx, dy, d;
                float kd, ndx, ndy;

                // Find first not short segment
                width          *= 0.5f;
                size_t si       = 0;
                for (i = 1; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                        break;
                }
                if (i >= n)
                    return;

                // Draw first segment
                kd              = width / sqrtf(d);
                ndx             = -dy * kd;
                ndy             = dx * kd;

                ADD_VERTEX(v, ci, x[i] + ndx, y[i] + ndy);
                ADD_VERTEX(v, ci, x[i] - ndx, y[i] - ndy);
                ADD_VERTEX(v, ci, x[si] - ndx, y[si] - ndy);
                ADD_VERTEX(v, ci, x[si] + ndx, y[si] + ndy);

                ADD_HRECTANGLE(iv, vi, vi + 1, vi + 2, vi + 3);
                si                  = i++;

                // Draw the rest segments
                for (; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                    {
                        kd              = width / sqrtf(d);
                        ndx             = -dy * kd;
                        ndy             = dx * kd;

                        ADD_VERTEX(v, ci, x[i] + ndx, y[i] + ndy);
                        ADD_VERTEX(v, ci, x[i] - ndx, y[i] - ndy);
                        ADD_VERTEX(v, ci, x[si] - ndx, y[si] - ndy);
                        ADD_VERTEX(v, ci, x[si] + ndx, y[si] + ndy);

                        ADD_HRECTANGLE(iv, vi + 4, vi + 5, vi + 6, vi + 7);
                        ADD_HRECTANGLE(iv, vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }
            }

            template <class T>
            inline void Surface::draw_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n)
            {
                vertex_t *v     = vertices;
                T *iv           = indices;
                lsp_finally
                {
                    vertices        = v;
                    indices         = iv;
                };

                size_t i;
                float dx, dy, d;
                float kd, ndx, ndy;
                float px, py;

                // Find first not short segment
                width          *= 0.5f;
                size_t si       = 0;
                for (i = 1; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                        break;
                }
                if (i >= n)
                    return;

                // Draw first segment
                kd              = width / sqrtf(d);
                ndx             = -dy * kd;
                ndy             = dx * kd;

                px              = x[i] + ndx;
                py              = y[i] + ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[i] - ndx;
                py              = y[i] - ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[si] - ndx;
                py              = y[si] - ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[si] + ndx;
                py              = y[si] + ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                ADD_HRECTANGLE(iv, vi, vi + 1, vi + 2, vi + 3);
                si                  = i++;

                // Draw the rest segments
                for (; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                    {
                        kd              = width / sqrtf(d);
                        ndx             = -dy * kd;
                        ndy             = dx * kd;

                        px              = x[i] + ndx;
                        py              = y[i] + ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[i] - ndx;
                        py              = y[i] - ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[si] - ndx;
                        py              = y[si] - ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[si] + ndx;
                        py              = y[si] + ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        ADD_HRECTANGLE(iv, vi + 4, vi + 5, vi + 6, vi + 7);
                        ADD_HRECTANGLE(iv, vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }
            }

            void Surface::draw_polyline(uint32_t ci, const float *x, const float *y, float width, size_t n)
            {
                // Allocate vertices
                const uint32_t segs = n - 1;
                const uint32_t v_reserve = segs * 4;
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(v_reserve);
                if (v == NULL)
                    return;
                const vertex_t *v_tail = &v[v_reserve];
                lsp_finally {
                    if (v_tail > v)
                        sBatch.release_vertices(v_tail - v);
                };

                // Allocate indices
                const uint32_t iv_reserve = (2*segs - 1) * 6;
                void *iv_raw    = sBatch.add_indices(iv_reserve, vi + v_reserve - 1);
                if (iv_raw == NULL)
                    return;
                ssize_t iv_release = iv_reserve;
                lsp_finally {
                    if (iv_release > 0)
                        sBatch.release_indices(iv_release);
                };

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U8:
                    {
                        uint8_t *iv     = static_cast<uint8_t *>(iv_raw);
                        const uint8_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint8_t>(v, iv, uint8_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv    = static_cast<uint16_t *>(iv_raw);
                        const uint16_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint16_t>(v, iv, uint16_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv    = static_cast<uint32_t *>(iv_raw);
                        const uint32_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint32_t>(v, iv, uint32_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    default:
                        break;
                }
            }

            void Surface::draw_polyline(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n)
            {
                // Initialize rectangle of invalid size
                rect.left       = nWidth;
                rect.top        = nHeight;
                rect.right      = 0.0f;
                rect.bottom     = 0.0f;

                // Allocate vertices
                const uint32_t segs = n - 1;
                const uint32_t v_reserve = segs * 4;
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(v_reserve);
                if (v == NULL)
                    return;
                const vertex_t *v_tail = &v[v_reserve];
                lsp_finally {
                    if (v_tail > v)
                        sBatch.release_vertices(v_tail - v);
                };

                // Allocate indices
                const uint32_t iv_reserve = (2*segs - 1) * 6;
                void *iv_raw    = sBatch.add_indices(iv_reserve, vi + v_reserve - 1);
                if (iv_raw == NULL)
                    return;
                ssize_t iv_release = iv_reserve;
                lsp_finally {
                    if (iv_release > 0)
                        sBatch.release_indices(iv_release);
                };

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U8:
                    {
                        uint8_t *iv     = static_cast<uint8_t *>(iv_raw);
                        const uint8_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint8_t>(v, iv, uint8_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv    = static_cast<uint16_t *>(iv_raw);
                        const uint16_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint16_t>(v, iv, uint16_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv    = static_cast<uint32_t *>(iv_raw);
                        const uint32_t *iv_tail = &iv[iv_reserve];
                        draw_polyline<uint32_t>(v, iv, uint32_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    default:
                        break;
                }

                // Limit the rectangle
                limit_rect(rect);
            }

            bool Surface::valid() const
            {
                return (pContext != NULL) && (pContext->valid());
            }

            void Surface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                // Create texture
                if (!bIsDrawing)
                    return;
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                gl::Texture *t = gls->pTexture;
                if (t == NULL)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, a);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const uint32_t ci   = uint32_t(res);
                const float xe      = x + t->width() * sx;
                const float ye      = y + t->height() * sy;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_TVERTEX(v, ci, x, y, 0.0f, 1.0f);
                ADD_TVERTEX(v, ci, x, ye, 0.0f, 0.0f);
                ADD_TVERTEX(v, ci, xe, ye, 1.0f, 0.0f);
                ADD_TVERTEX(v, ci, xe, y, 1.0f, 1.0f);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                // Create texture
                if (!bIsDrawing)
                    return;
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                gl::Texture *t = gls->pTexture;
                if (t == NULL)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, a);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const uint32_t ci   = uint32_t(res);
                const float ca      = cosf(ra);
                const float sa      = sinf(ra);

                sx                 *= s->width();
                sy                 *= s->height();

                const float v1x     = ca * sx;
                const float v1y     = sa * sx;
                const float v2x     = -sa * sy;
                const float v2y     = ca * sy;

                // Draw picture
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_TVERTEX(v, ci, x, y, 0.0f, 1.0f);
                ADD_TVERTEX(v, ci, x + v2x, y + v2y, 0.0f, 0.0f);
                ADD_TVERTEX(v, ci, x + v1x + v2x, y + v1y + v2y, 1.0f, 0.0f);
                ADD_TVERTEX(v, ci, x + v1x, y + v1y, 1.0f, 1.0f);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a)
            {
                // Create texture
                if (!bIsDrawing)
                    return;

                if (s->type() != ST_OPENGL)
                    return;
                gl::Surface *gls = static_cast<gl::Surface *>(s);
                gl::Texture *t = gls->pTexture;
                if (t == NULL)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, a);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const float kw      = 1.0f / t->width();
                const float kh      = 1.0f / t->height();
                const uint32_t ci   = uint32_t(res);
                const float xe      = x + sw;
                const float ye      = y + sh;
                const float sxb     = sx * kw;
                const float syb     = sy * kh;
                const float sxe     = (sx + sw) * kw;
                const float sye     = (sy + sh) * kh;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_TVERTEX(v, ci, x, y, sxb, sye);
                ADD_TVERTEX(v, ci, x, ye, sxb, syb);
                ADD_TVERTEX(v, ci, xe, ye, sxe, syb);
                ADD_TVERTEX(v, ci, xe, y, sxe, sye);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::draw_raw(
                const void *data, size_t width, size_t height, size_t stride,
                float x, float y, float sx, float sy, float a)
            {
                // Create texture
                if (!bIsDrawing)
                    return;

                // Activate context
                if (pContext->activate() != STATUS_OK)
                    return;

                gl::Texture *tex = new gl::Texture(pContext);
                if (tex == NULL)
                    return;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                if (tex->set_image(data, width, height, stride, TEXTURE_PRGBA32) != STATUS_OK)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, tex, a);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const uint32_t ci   = uint32_t(res);
                const float xe      = x + width * sx;
                const float ye      = y + height * sy;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_TVERTEX(v, ci, x, y, 0.0f, 0.0f);
                ADD_TVERTEX(v, ci, x, ye, 0.0f, 1.0f);
                ADD_TVERTEX(v, ci, xe, ye, 1.0f, 1.0f);
                ADD_TVERTEX(v, ci, xe, y, 1.0f, 0.0f);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            status_t Surface::resize(size_t width, size_t height)
            {
                nWidth      = width;
                nHeight     = height;

                if (pTexture != NULL)
                {
                    status_t res = pTexture->resize(width, height);
                    if (res != STATUS_OK)
                        safe_release(pTexture);
                }

                sync_matrix();

                return STATUS_OK;
            }

            void Surface::begin()
            {
                if (pContext == NULL)
                    return;

                // Force end() call
                end();

                // Activate GLX context
                if (bNested)
                    bIsDrawing  = true;
                else
                {
                    if (pContext->activate() == STATUS_OK)
                        bIsDrawing  = true;
                    OPENGL_OUTPUT_STATS(false);
                }

                sBatch.clear();

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            bool Surface::update_uniforms()
            {
                vUniforms.clear();
                gl::uniform_t *model = vUniforms.add();
                if (model == NULL)
                    return false;

                gl::uniform_t *end = vUniforms.add();
                if (end == NULL)
                    return false;


                model->name     = "u_model";
                model->type     = gl::UNI_MAT4F;
                model->f32      = vMatrix;

                end->name       = NULL;
                end->type       = gl::UNI_NONE;
                end->raw        = NULL;

                return true;
            }

            void Surface::end()
            {
                // Update drawing status
                if (!bIsDrawing)
                    return;
                lsp_finally
                {
                    sBatch.clear();
                    bIsDrawing = false;
                };

                // Update uniforms
                if (!update_uniforms())
                    return;

                #ifdef LSP_DEBUG
                    if (nNumClips > 0)
                        lsp_error("Mismatching number of clip_begin() and clip_end() calls");
                #endif /* LSP_DEBUG */

                // Activate OpenGL context for drawing
                if (pContext->activate() != STATUS_OK)
                    return;
                lsp_finally {
                    if (!bNested)
                    {
                        pText->clear();
                        pContext->deactivate();
                    }
                };

                // Perform rendering of the collected batch
                const gl::vtbl_t *vtbl = pContext->vtbl();

                if (bNested)
                {
                    // Ensure that texture is properly initialized
                    if (pTexture == NULL)
                    {
                        pTexture        = new gl::Texture(pContext);
                        if (pTexture == NULL)
                            return;
                    }

                    // Setup texture for drawing
                    status_t res = pTexture->begin_draw(nWidth, nHeight, gl::TEXTURE_PRGBA32);
                    if (res != STATUS_OK)
                        return;
                    lsp_finally { pTexture->end_draw(); };

                    // Execute batch
                    vtbl->glViewport(0, 0, nWidth, nHeight);
                    sBatch.execute(pContext, vUniforms.array());
                }
                else
                {
                    // Setup viewport
                    const ssize_t height = pContext->height();
                    vtbl->glViewport(0, height - nHeight, nWidth, nHeight);

                    // Set drawing buffer and viewport
                    vtbl->glDrawBuffer(GL_BACK);

                    // Execute batch
                    sBatch.execute(pContext, vUniforms.array());

                    // Instead of swapping buffers we copy back buffer to front buffer to prevent the back buffer image
                    pContext->swap_buffers(nWidth, nHeight);
                }
            }

            void Surface::clear_rgb(uint32_t rgb)
            {
                // Start batch
                const ssize_t res = start_batch(
                    gl::GEOMETRY,
                    gl::BATCH_WRITE_COLOR,
                    float((rgb >> 16) & 0xff) * k_color,
                    float((rgb >> 8) & 0xff) * k_color,
                    float(rgb & 0xff) * k_color,
                    0.0f);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_rect(uint32_t(res), 0.0f, 0.0f, nWidth, nHeight);
            }

            void Surface::clear_rgba(uint32_t rgba)
            {
                // Start batch
                const ssize_t res = start_batch(
                    gl::GEOMETRY,
                    gl::BATCH_WRITE_COLOR,
                    float((rgba >> 16) & 0xff) * k_color,
                    float((rgba >> 8) & 0xff) * k_color,
                    float(rgba & 0xff) * k_color,
                    float((rgba >> 24) & 0xff) * k_color);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_rect(uint32_t(res), 0.0f, 0.0f, nWidth, nHeight);
            }

            void Surface::clear(const Color &c)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_NO_BLENDING, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_rect(uint32_t(res), 0.0f, 0.0f, nWidth, nHeight);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, left, top, width, height, line_width);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight, line_width);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight, line_width);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, left, top, width, height, line_width);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, left, top, width, height);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, left, top, width, height);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void Surface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, float left, float top, float width, float height)
            {
                // Create texture
                if (!bIsDrawing)
                    return;
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                gl::Texture *t = gls->pTexture;
                if (t == NULL)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, alpha);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                texcoord_t tex;
                tex.x       = left;
                tex.y       = top + height;
                tex.sx      = 1.0f / width;
                tex.sy      = -1.0f / height;

                fill_textured_rect(uint32_t(res), tex, mask, radius, left, top, width, height);
            }

            void Surface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // Create texture
                if (!bIsDrawing)
                    return;
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                gl::Texture *t = gls->pTexture;
                if (t == NULL)
                    return;

                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, alpha);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                texcoord_t tex;
                tex.x       = r->nLeft;
                tex.y       = r->nTop + r->nHeight;
                tex.sx      = 1.0f / r->nWidth;
                tex.sy      = -1.0f / r->nHeight;

                fill_textured_rect(uint32_t(res), tex, mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void Surface::fill_sector(const Color &c, float x, float y, float r, float a1, float a2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_sector(uint32_t(res), x, y, r, a1, a2);
            }

            void Surface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_triangle(uint32_t(res), x0, y0, x1, y1, x2, y2);
            }

            void Surface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_triangle(uint32_t(res), x0, y0, x1, y1, x2, y2);
            }

            void Surface::line(const Color & c, float x0, float y0, float x1, float y1, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                draw_line(uint32_t(res), x0, y0, x1, y1, width);
            }

            void Surface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                draw_line(uint32_t(res), x0, y0, x1, y1, width);
            }

            void Surface::parametric_line(const Color & color, float a, float b, float c, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, color);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw the line
                if (fabs(a) > fabs(b))
                    draw_line(uint32_t(res), - c / a, 0.0f, -(c + b*nHeight)/a, nHeight, width);
                else
                    draw_line(uint32_t(res), 0.0f, - c / b, nWidth, -(c + a*nWidth)/b, width);
            }

            void Surface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, color);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw the line
                if (fabs(a) > fabs(b))
                    draw_line(uint32_t(res),
                        roundf(-(c + b*top)/a), roundf(top),
                        roundf(-(c + b*bottom)/a), roundf(bottom),
                        width);
                else
                    draw_line(uint32_t(res),
                        roundf(left), roundf(-(c + a*left)/b),
                        roundf(right), roundf(-(c + a*right)/b),
                        width);
            }

            void Surface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw the primitive
                const uint32_t ci   = uint32_t(res);
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                if (fabs(a1) > fabs(b1))
                {
                    ADD_VERTEX(v, ci, -(c1 + b1*top)/a1, top);
                    ADD_VERTEX(v, ci, -(c1 + b1*bottom)/a1, bottom);
                }
                else
                {
                    ADD_VERTEX(v, ci, left, -(c1 + a1*left)/b1);
                    ADD_VERTEX(v, ci, right, -(c1 + a1*right)/b1);
                }

                if (fabs(a2) > fabs(b2))
                {
                    ADD_VERTEX(v, ci, -(c2 + b2*bottom)/a2, bottom);
                    ADD_VERTEX(v, ci, -(c2 + b2*top)/a2, top);
                }
                else
                {
                    ADD_VERTEX(v, ci, right, -(c2 + a2*right)/b2);
                    ADD_VERTEX(v, ci, left, -(c2 + a2*left)/b2);
                }

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Surface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                wire_arc(uint32_t(res), x, y, r, a1, a2, width);
            }

            void Surface::fill_poly(const Color & c, const float *x, const float *y, size_t n)
            {
                // Some optimizations
                if (n <= 3)
                {
                    if (n == 3)
                    {
                        // Start batch
                        const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                        if (res < 0)
                            return;
                        lsp_finally { sBatch.end(); };

                        // Draw geometry
                        fill_triangle(uint32_t(res), x[0], y[0], x[1], y[1], x[2], y[2]);
                    }
                    return;
                }

                // Start first batch on stencil buffer
                clip_rect_t rect;
                {
                    const ssize_t res = start_batch(gl::STENCIL, gl::BATCH_STENCIL_OP_XOR | gl::BATCH_CLEAR_STENCIL, 0.0f, 0.0f, 0.0f, 0.0f);
                    if (res < 0)
                        return;
                    lsp_finally{ sBatch.end(); };

                    fill_triangle_fan(size_t(res), rect, x, y, n);
                }

                // Start second batch on color buffer with stencil apply
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_STENCIL_OP_APPLY, c);
                    if (res < 0)
                        return;
                    lsp_finally{ sBatch.end(); };

                    fill_rect(size_t(res), rect.left, rect.top, rect.right, rect.bottom);
                }
            }

            void Surface::fill_poly(IGradient *g, const float *x, const float *y, size_t n)
            {
                // Some optimizations
                if (n <= 3)
                {
                    if (n == 3)
                    {
                        // Start batch
                        const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                        if (res < 0)
                            return;
                        lsp_finally { sBatch.end(); };

                        // Draw geometry
                        fill_triangle(uint32_t(res), x[0], y[0], x[1], y[1], x[2], y[2]);
                    }
                    return;
                }

                // Start first batch on stencil buffer
                clip_rect_t rect;
                {
                    const ssize_t res = start_batch(gl::STENCIL, gl::BATCH_STENCIL_OP_XOR | gl::BATCH_CLEAR_STENCIL, 0.0f, 0.0f, 0.0f, 0.0f);
                    if (res < 0)
                        return;
                    lsp_finally{ sBatch.end(); };

                    fill_triangle_fan(size_t(res), rect, x, y, n);
                }

                // Start second batch on color buffer with stencil apply
                {
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_STENCIL_OP_APPLY, g);
                    if (res < 0)
                        return;
                    lsp_finally{ sBatch.end(); };

                    fill_rect(size_t(res), rect.left, rect.top, rect.right, rect.bottom);
                }
            }

            void Surface::wire_poly(const Color & c, float width, const float *x, const float *y, size_t n)
            {
                if (width < 1e-6f)
                    return;

                if (n <= 2)
                {
                    if (n == 2)
                    {
                        // Start batch
                        const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                        if (res < 0)
                            return;
                        lsp_finally { sBatch.end(); };

                        // Draw geometry
                        draw_line(uint32_t(res), x[0], y[0], x[1], y[1], width);
                    }
                    return;
                }

                if (c.alpha() < k_color)
                {
                    // Opaque polyline can be drawin without stencil buffer
                    const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                    if (res < 0)
                        return;
                    lsp_finally{ sBatch.end(); };

                    draw_polyline(size_t(res), x, y, width, n);
                }
                else
                {
                    // Start first batch on stencil buffer
                    clip_rect_t rect;
                    {
                        const ssize_t res = start_batch(gl::STENCIL, gl::BATCH_STENCIL_OP_OR | gl::BATCH_CLEAR_STENCIL, 0.0f, 0.0f, 0.0f, 0.0f);
                        if (res < 0)
                            return;
                        lsp_finally{ sBatch.end(); };

                        draw_polyline(size_t(res), rect, x, y, width, n);
                    }

                    // Start second batch on color buffer with stencil apply
                    {
                        const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_STENCIL_OP_APPLY, c);
                        if (res < 0)
                            return;
                        lsp_finally{ sBatch.end(); };

                        fill_rect(size_t(res), rect.left, rect.top, rect.right, rect.bottom);
                    }
                }
            }

            void Surface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                fill_poly(fill, x, y, n);
                wire_poly(wire, width, x, y, n);
            }

            void Surface::fill_circle(const Color &c, float x, float y, float r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_circle(uint32_t(res), x, y, r);
            }

            void Surface::fill_circle(IGradient *g, float x, float y, float r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_circle(uint32_t(res), x, y, r);
            }

            void Surface::fill_frame(
                const Color &c,
                size_t flags, float radius,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_frame(
                    uint32_t(res), flags, radius,
                    fx, fy, fw, fh,
                    ix, iy, iw, ih);
            }

            void Surface::fill_frame(
                const Color &c,
                size_t flags, float radius,
                const ws::rectangle_t *out, const ws::rectangle_t *in)
            {
                // Start batch
                const ssize_t res = start_batch(gl::GEOMETRY, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_frame(
                    uint32_t(res), flags, radius,
                    out->nLeft, out->nTop, out->nWidth, out->nHeight,
                    in->nLeft, in->nTop, in->nWidth, in->nHeight);
            }

            bool Surface::get_antialiasing()
            {
                return bAntiAliasing;
            }

            bool Surface::set_antialiasing(bool set)
            {
                const bool old = bAntiAliasing;
                bAntiAliasing       = set;
                return old;
            }

            ws::point_t Surface::set_origin(const ws::point_t & origin)
            {
                ws::point_t result;
                result.nLeft    = sOrigin.left;
                result.nTop     = sOrigin.top;

                sOrigin.left    = int32_t(origin.nLeft);
                sOrigin.top     = int32_t(origin.nTop);

                return result;
            }

            ws::point_t Surface::set_origin(ssize_t left, ssize_t top)
            {
                ws::point_t result;
                result.nLeft    = sOrigin.left;
                result.nTop     = sOrigin.top;

                sOrigin.left    = int32_t(left);
                sOrigin.top     = int32_t(top);

                return result;
            }

            void Surface::clip_begin(float x, float y, float w, float h)
            {
                if (!bIsDrawing)
                    return;

                if (nNumClips >= MAX_CLIPS)
                {
                    lsp_error("Too many clipping regions specified (%d)", int(nNumClips + 1));
                    return;
                }

                clip_rect_t *rect = &vClips[nNumClips++];
                rect->left      = x;
                rect->top       = y;
                rect->right     = x + w;
                rect->bottom    = y + h;
            }

            void Surface::clip_end()
            {
                if (!bIsDrawing)
                    return;

                if (nNumClips <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                --nNumClips;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

