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
                nWidth          = width;
                nHeight         = height;
                nNumClips       = 0;
                bNested         = false;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);

                sBatch.init();
            }

            Surface::Surface(size_t width, size_t height)
            {
                pDisplay        = NULL;
                pContext        = NULL;
                nWidth          = width;
                nHeight         = height;
                bNested         = true;
                nNumClips       = 0;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                bzero(vMatrix, sizeof(float) * 16);
                bzero(vClips, sizeof(clip_rect_t) * MAX_CLIPS);

                sBatch.init();
            }

            IDisplay *Surface::display()
            {
                return pDisplay;
            }

            ISurface *Surface::create(size_t width, size_t height)
            {
                Surface *s = new Surface(width, height);
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
                    dst[0]      = r->left;
                    dst[1]      = r->top;
                    dst[2]      = r->right;
                    dst[3]      = r->bottom;

                    dst        += 4;
                }

                return dst;
            }

            inline float *Surface::serialize_color(float *dst, float r, float g, float b, float a)
            {
                dst[0]      = r;
                dst[1]      = g;
                dst[2]      = b;
                dst[3]      = a;

                return dst + 4;
            }

            inline float *Surface::serialize_color(float *dst, const Color & c)
            {
                dst[0]      = c.red();
                dst[1]      = c.green();
                dst[2]      = c.blue();
                dst[3]      = c.alpha();

                return dst + 4;
            }

            ssize_t Surface::start_batch(batch_program_t program, uint32_t flags, float r, float g, float b, float a)
            {
                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        flags,
                        bAntiAliasing,
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

            ssize_t Surface::start_batch(batch_program_t program, uint32_t flags, const Color & color)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        flags,
                        bAntiAliasing,
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

            ssize_t Surface::start_batch(batch_program_t program, uint32_t flags, const IGradient * g)
            {
                if (!bIsDrawing)
                    return -STATUS_BAD_STATE;
                if (g == NULL)
                    return -STATUS_BAD_ARGUMENTS;

                // Start batch
                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        flags,
                        bAntiAliasing,
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

            void Surface::fill_circle(uint32_t ci, float x, float y, float r)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float alpha = lsp_min(4.0f / r, M_PI_2);
                const float dx = cosf(alpha);
                const float dy = sinf(alpha);
                const size_t count = M_PI * 0.5f * r;

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

                const float alpha = (delta > 0.0f) ? lsp_min(4.0f / r, M_PI_2) : lsp_min(-4.0f / r, M_PI_2);
                const float ex  = cosf(a2) * r;
                const float ey  = sinf(a2) * r;
                const float dx  = cosf(alpha);
                const float dy  = sinf(alpha);
                const ssize_t count = delta / alpha;

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

                const float delta = M_PI * 0.5f;
                const float alpha = (delta > 0.0f) ? lsp_min(4.0f / r, M_PI_2) : lsp_min(-4.0f / r, M_PI_2);
                const float dx  = cosf(alpha);
                const float dy  = sinf(alpha);
                const ssize_t count = delta / alpha;

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

                const float hw  = width * 0.5f;
                const float ro  = r + hw;
                const float kr  = lsp_max(r - hw, 0.0f) / ro;

                const float alpha = (delta > 0.0f) ? lsp_min(4.0f / ro, M_PI_2) : lsp_min(-4.0f / ro, M_PI_2);
                const float ex  = cosf(a2) * ro;
                const float ey  = sinf(a2) * ro;

                const float dx  = cosf(alpha);
                const float dy  = sinf(alpha);
                const ssize_t count = delta / alpha;

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

            bool Surface::valid() const
            {
                return pContext != NULL;
            }

            void Surface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                // TODO
            }

            void Surface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                // TODO
            }

            void Surface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a)
            {
                // TODO
            }

            void Surface::draw_raw(
                const void *data, size_t width, size_t height, size_t stride,
                float x, float y, float sx, float sy, float a)
            {
                // TODO
            }

            void Surface::begin()
            {
                if (pContext == NULL)
                    return;

                // Force end() call
                end();

                if (bNested)
                {
                }
                else
                {
                    pContext->activate();
                    glViewport(0, 0, nWidth, nHeight);

                    int max_texture_size = 0;
                    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
                    lsp_trace("Max texture size = %d", max_texture_size);
                }

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

                // Set-up antialiasing
                if (bAntiAliasing)
                    glEnable(GL_MULTISAMPLE);
                else
                    glDisable(GL_MULTISAMPLE);

                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(vMatrix);
                glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
                glEnable(GL_BLEND);

                bIsDrawing      = true;

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            void Surface::end()
            {
                if (!bIsDrawing)
                    return;
                if (pContext == NULL)
                    return;

            #ifdef LSP_DEBUG
                if (nNumClips > 0)
                    lsp_error("Mismatching number of clip_begin() and clip_end() calls");
            #endif /* LSP_DEBUG */

                lltl::darray<gl::uniform_t> vUniforms;
                vUniforms.add(gl::uniform_t { "u_model", gl::UNI_MAT4F, vMatrix });
                vUniforms.add(gl::uniform_t { NULL, gl::UNI_NONE, NULL });

                // Execute batch
                sBatch.execute(pContext, vUniforms.array());

                if (bNested)
                {
                }
                else
                {
//                    glReadBuffer(GL_BACK);
//                    glDrawBuffer(GL_FRONT);
//                    glCopyPixels(0, 0, nWidth, nHeight, GL_COLOR);
//                    glReadBuffer(GL_FRONT);
//                    glDrawBuffer(GL_BACK);
                    pContext->deactivate();
                }

                bIsDrawing      = false;
            }

            void Surface::clear_rgb(uint32_t rgb)
            {
                // Start batch
                const ssize_t res = start_batch(
                    gl::SIMPLE,
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
                    gl::SIMPLE,
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
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_rect(uint32_t(res), 0.0f, 0.0f, nWidth, nHeight);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, left, top, width, height, line_width);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight, line_width);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight, line_width);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                wire_rect(uint32_t(res), mask, radius, left, top, width, height, line_width);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, left, top, width, height);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, left, top, width, height);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw primitives
                fill_rect(uint32_t(res), mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
            }

            void Surface::fill_sector(const Color &c, float x, float y, float r, float a1, float a2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_sector(uint32_t(res), x, y, r, a1, a2);
            }

            void Surface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_triangle(uint32_t(res), x0, y0, x1, y1, x2, y2);
            }

            void Surface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_triangle(uint32_t(res), x0, y0, x1, y1, x2, y2);
            }

            bool Surface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // TODO

                return false;
            }

            bool Surface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                if (text == NULL)
                    return false;

                // TODO

                return true;
            }

            bool Surface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (text == NULL)
                    return false;

                // TODO

                return true;
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                // TODO
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                // TODO
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                // TODO
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                // TODO
            }

            void Surface::line(const Color &color, float x0, float y0, float x1, float y1, float width)
            {
                // TODO
            }

            void Surface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                // TODO
            }

            void Surface::parametric_line(const Color &color, float a, float b, float c, float width)
            {
                // TODO
            }

            void Surface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                // TODO
            }

            void Surface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                // TODO
            }

            void Surface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                wire_arc(uint32_t(res), x, y, r, a1, a2, width);
            }

            void Surface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
                // TODO
            }

            void Surface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
                // TODO
            }

            void Surface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
                // TODO
            }

            void Surface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                // TODO
            }

            void Surface::fill_circle(const Color &c, float x, float y, float r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
                if (res < 0)
                    return;
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_circle(uint32_t(res), x, y, r);
            }

            void Surface::fill_circle(IGradient *g, float x, float y, float r)
            {
                // Start batch
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, g);
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
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
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
                const ssize_t res = start_batch(gl::SIMPLE, gl::BATCH_WRITE_COLOR, c);
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
                if (bAntiAliasing == set)
                    return bAntiAliasing;

                bAntiAliasing       = set;
                if (bIsDrawing)
                {
                    if (bAntiAliasing)
                        glEnable(GL_MULTISAMPLE);
                    else
                        glDisable(GL_MULTISAMPLE);
                }

                return !bAntiAliasing;
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

