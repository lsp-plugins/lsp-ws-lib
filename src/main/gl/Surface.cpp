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

#if defined(USE_LIBX11)

#ifndef USE_LIBFREETYPE
#error "Freetype is required to render text for X11 OpenGL surface"
#endif /* USE_LIBFREETYPE */

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <private/freetype/FontManager.h>
#include <private/gl/Batch.h>
#include <private/gl/Gradient.h>
#include <private/gl/Surface.h>
#include <private/x11/X11Display.h>

#include <GL/gl.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            constexpr float k_color = 1.0f / 255.0f;

            Surface::Surface(IDisplay *display, gl::IContext *ctx, size_t width, size_t height):
                ISurface(width, height, ST_OPENGL)
            {
                pDisplay        = display;
                pContext        = safe_acquire(ctx);
                pTexture        = NULL;
                nWidth          = width;
                nHeight         = height;
                nNumClips       = 0;
                bNested         = false;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);

                sBatch.init();
                sync_matrix();

                lsp_trace("primary surface created ptr=%p", this);
            }

            Surface::Surface(size_t width, size_t height):
                ISurface(width, height, ST_OPENGL)
            {
                pDisplay        = NULL;
                pContext        = NULL;
                pTexture        = NULL;
                nWidth          = width;
                nHeight         = height;
                bNested         = true;
                nNumClips       = 0;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);

                sBatch.init();
                sync_matrix();
            }

            Surface *Surface::create_nested(size_t width, size_t height)
            {
                return new Surface(width, height);
            }

            IDisplay *Surface::display()
            {
                return pDisplay;
            }

            ISurface *Surface::create(size_t width, size_t height)
            {
                Surface *s = create_nested(width, height);
                if (s != NULL)
                {
                    s->pDisplay     = pDisplay;
                    s->pContext     = safe_acquire(pContext);
                }

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
                safe_release(pContext);

                pDisplay        = NULL;
                pContext        = NULL;
            }

            Surface::~Surface()
            {
                do_destroy();
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
                dst[0]      = float(t->width());
                dst[1]      = float(t->height());
                dst[2]      = t->format();
                dst[3]      = t->multisampling();

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
                rect.left           = lsp_max(rect.left, 0.0f);
                rect.top            = lsp_max(rect.top, 0.0f);
                rect.right          = lsp_min(rect.right, float(nWidth));
                rect.bottom         = lsp_min(rect.bottom, float(nHeight));
            }

            ssize_t Surface::start_batch(gl::program_t program, uint32_t flags, float r, float g, float b, float a)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
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
                buf     = serialize_texture(buf, t);

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
                buf     = serialize_texture(buf, t);

                return make_command(index, C_TEXTURE);
            }

            void Surface::fill_triangle(uint32_t ci, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                const ssize_t vi    = sBatch.vertex(ci, x0, y0);
                sBatch.vertex(ci, x1, y1);
                sBatch.vertex(ci, x2, y2);
                sBatch.triangle(vi, vi + 1, vi + 2);
            }

            void Surface::fill_rect(uint32_t ci, float x0, float y0, float x1, float y1)
            {
                const ssize_t vi    = sBatch.vertex(ci, x0, y0);
                sBatch.vertex(ci, x0, y1);
                sBatch.vertex(ci, x1, y1);
                sBatch.vertex(ci, x1, y0);
                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
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

                const ssize_t vi = sBatch.vertex(ci, x0 + ndx, y0 + ndy);
                sBatch.vertex(ci, x0 - ndx, y0 - ndy);
                sBatch.vertex(ci, x1 - ndx, y1 - ndy);
                sBatch.vertex(ci, x1 + ndx, y1 + ndy);
                sBatch.rectangle(vi, vi+1, vi+2, vi+3);
            }

            void Surface::fill_triangle_fan(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, size_t n)
            {
                if (n < 3)
                    return;

                const ssize_t v0i   = sBatch.vertex(ci, x[0], y[0]);
                ssize_t vi          = sBatch.vertex(ci, x[1], y[1]);

                rect.left           = lsp_min(x[0], x[1]);
                rect.top            = lsp_min(y[0], y[1]);
                rect.right          = lsp_max(x[0], x[1]);
                rect.bottom         = lsp_max(y[0], y[1]);

                for (size_t i=2; i<n; ++i)
                {
                    extend_rect(rect, x[i], y[i]);
                    sBatch.vertex(ci, x[i], y[i]);
                    sBatch.triangle(v0i, vi, vi + 1);
                    ++vi;
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

                // Fill batch
                float vx = r;
                float vy = 0.0f;

                const ssize_t v0i   = sBatch.vertex(ci, x, y);
                size_t v1i          = sBatch.vertex(ci, x + vx, y + vy);

                for (size_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(ci, x + vx, y + vy);
                    sBatch.triangle(v0i, v1i, v1i + 1);
                    ++v1i;
                }

                sBatch.vertex(ci, x + r, y);
                sBatch.triangle(v0i, v1i, v1i + 1);
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

                // Generate the geometry
                float vx            = cosf(a1) * r;
                float vy            = sinf(a1) * r;

                const size_t v0i    = sBatch.vertex(ci, x, y);
                size_t v1i          = sBatch.vertex(ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(ci, x + vx, y + vy);
                    sBatch.triangle(v0i, v1i, v1i + 1);
                    ++v1i;
                }

                sBatch.vertex(ci, x + ex, y + ey);
                sBatch.triangle(v0i, v1i, v1i + 1);
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

                // Generate the geometry
                float vx            = cosf(a) * r;
                float vy            = sinf(a) * r;
                const float ex      = -vy;
                const float ey      = vx;

                const size_t v0i    = sBatch.vertex(ci, xd, yd);
                size_t v1i          = sBatch.vertex(ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(ci, x + vx, y + vy);
                    sBatch.triangle(v0i, v1i, v1i + 1);
                    ++v1i;
                }

                sBatch.vertex(ci, x + ex, y + ey);
                sBatch.triangle(v0i, v1i, v1i + 1);
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

                // Fill batch
                float vx        = cosf(a1) * ro;
                float vy        = sinf(a1) * ro;

                ssize_t v0i         = sBatch.vertex(ci, x + vx * kr, y + vy * kr);
                sBatch.vertex(ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(ci, x + vx * kr, y + vy * kr);
                    sBatch.vertex(ci, x + vx, y + vy);
                    sBatch.rectangle(v0i, v0i + 1, v0i + 3, v0i + 2);
                    v0i        += 2;
                }

                sBatch.vertex(ci, x + ex * kr, y + ey * kr);
                sBatch.vertex(ci, x + ex, y + ey);
                sBatch.rectangle(v0i, v0i + 1, v0i + 3, v0i + 2);
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
                    fill_rect(ci, fx, fy, fw, fh);
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

            void Surface::draw_polyline(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n)
            {
                size_t i;
                float dx, dy, d;
                float kd, ndx, ndy;
                float px, py;

                rect.left       = nWidth;
                rect.top        = nHeight;
                rect.right      = 0.0f;
                rect.bottom     = 0.0f;

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
                ssize_t vi      = sBatch.vertex(ci, px, py);

                px              = x[i] - ndx;
                py              = y[i] - ndy;
                extend_rect(rect, px, py);
                sBatch.vertex(ci, px, py);

                px              = x[si] - ndx;
                py              = y[si] - ndy;
                extend_rect(rect, px, py);
                sBatch.vertex(ci, px, py);

                px              = x[si] + ndx;
                py              = y[si] + ndy;
                extend_rect(rect, px, py);
                sBatch.vertex(ci, px, py);

                sBatch.rectangle(vi, vi+1, vi+2, vi+3);
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
                        sBatch.vertex(ci, px, py);

                        px              = x[i] - ndx;
                        py              = y[i] - ndy;
                        extend_rect(rect, px, py);
                        sBatch.vertex(ci, px, py);

                        px              = x[si] - ndx;
                        py              = y[si] - ndy;
                        extend_rect(rect, px, py);
                        sBatch.vertex(ci, px, py);

                        px              = x[si] + ndx;
                        py              = y[si] + ndy;
                        extend_rect(rect, px, py);
                        sBatch.vertex(ci, px, py);

                        sBatch.rectangle(vi + 4, vi + 5, vi + 6, vi + 7);
                        sBatch.rectangle(vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }

                // Limit the rectangle
                limit_rect(rect);
            }

            void Surface::draw_polyline(uint32_t ci, const float *x, const float *y, float width, size_t n)
            {
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

                ssize_t vi      = sBatch.vertex(ci, x[i] + ndx, y[i] + ndy);
                sBatch.vertex(ci, x[i] - ndx, y[i] - ndy);
                sBatch.vertex(ci, x[si] - ndx, y[si] - ndy);
                sBatch.vertex(ci, x[si] + ndx, y[si] + ndy);

                sBatch.rectangle(vi, vi+1, vi+2, vi+3);
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

                        sBatch.vertex(ci, x[i] + ndx, y[i] + ndy);
                        sBatch.vertex(ci, x[i] - ndx, y[i] - ndy);
                        sBatch.vertex(ci, x[si] - ndx, y[si] - ndy);
                        sBatch.vertex(ci, x[si] + ndx, y[si] + ndy);

                        sBatch.rectangle(vi + 4, vi + 5, vi + 6, vi + 7);
                        sBatch.rectangle(vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }
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

                const ssize_t vi    = sBatch.textured_vertex(ci, x, y, 0.0f, 1.0f);
                sBatch.textured_vertex(ci, x, ye, 0.0f, 0.0f);
                sBatch.textured_vertex(ci, xe, ye, 1.0f, 0.0f);
                sBatch.textured_vertex(ci, xe, y, 1.0f, 1.0f);
                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
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
                const ssize_t vi    = sBatch.textured_vertex(ci, x, y, 0.0f, 1.0f);
                sBatch.textured_vertex(ci, x + v2x, y + v2y, 0.0f, 0.0f);
                sBatch.textured_vertex(ci, x + v1x + v2x, y + v1y + v2y, 1.0f, 0.0f);
                sBatch.textured_vertex(ci, x + v1x, y + v1y, 1.0f, 1.0f);
                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
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

                const ssize_t vi    = sBatch.textured_vertex(ci, x, y, sxb, sye);
                sBatch.textured_vertex(ci, x, ye, sxb, syb);
                sBatch.textured_vertex(ci, xe, ye, sxe, syb);
                sBatch.textured_vertex(ci, xe, y, sxe, sye);
                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
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

                const ssize_t vi    = sBatch.textured_vertex(ci, x, y, 0.0f, 0.0f);
                sBatch.textured_vertex(ci, x, ye, 0.0f, 1.0f);
                sBatch.textured_vertex(ci, xe, ye, 1.0f, 1.0f);
                sBatch.textured_vertex(ci, xe, y, 1.0f, 0.0f);
                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            status_t Surface::resize(size_t width, size_t height)
            {
                nWidth      = width;
                nHeight     = height;

                safe_release(pTexture);

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
                        pContext->deactivate();
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
                    // Set drawing buffer and viewport
                    vtbl->glDrawBuffer(GL_BACK);

                    // Execute batch
                    vtbl->glViewport(0, 0, nWidth, nHeight);
                    sBatch.execute(pContext, vUniforms.array());

                    // Instead of swapping buffers we copy back buffer to front buffer to prevent the back buffer image
                    vtbl->glReadBuffer(GL_BACK);
                    vtbl->glDrawBuffer(GL_FRONT);
                    vtbl->glBlitFramebuffer(
                        0, 0, nWidth, nHeight,
                        0, 0, nWidth, nHeight,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
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
                const ssize_t vi    = sBatch.next_vertex_index();
                const uint32_t ci   = uint32_t(res);
                if (fabs(a1) > fabs(b1))
                {
                    sBatch.vertex(ci, -(c1 + b1*top)/a1, top);
                    sBatch.vertex(ci, -(c1 + b1*bottom)/a1, bottom);
                }
                else
                {
                    sBatch.vertex(ci, left, -(c1 + a1*left)/b1);
                    sBatch.vertex(ci, right, -(c1 + a1*right)/b1);
                }

                if (fabs(a2) > fabs(b2))
                {
                    sBatch.vertex(ci, -(c2 + b2*bottom)/a2, bottom);
                    sBatch.vertex(ci, -(c2 + b2*top)/a2, top);
                }
                else
                {
                    sBatch.vertex(ci, right, -(c2 + a2*right)/b2);
                    sBatch.vertex(ci, left, -(c2 + a2*left)/b2);
                }

                sBatch.rectangle(vi, vi + 1, vi + 2, vi + 3);
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
                    fx, fy, fw, fh,ix, iy, iw, ih);
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

#endif /* defined(USE_LIBX11) */

