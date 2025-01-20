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
#include <private/gl/Surface.h>
#include <private/x11/X11Display.h>

#include <GL/gl.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            Surface::Surface(IDisplay *display, gl::IContext *ctx, size_t width, size_t height):
                ISurface(width, height, ST_OPENGL)
            {
                pDisplay        = display;
                pContext        = safe_acquire(ctx);
                nWidth          = width;
                nHeight         = height;
                bNested         = false;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                sBatch.init();

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            Surface::Surface(size_t width, size_t height)
            {
                pDisplay        = NULL;
                pContext        = NULL;
                nWidth          = width;
                nHeight         = height;
                bNested         = true;
                bIsDrawing      = false;
                bAntiAliasing   = true;

                sBatch.init();

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
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
                // TODO
                return NULL;
            }

            IGradient *Surface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
            {
                // TODO
                return NULL;
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
                // TODO
            }

            void Surface::clear_rgba(uint32_t rgba)
            {
                // TODO
            }

            void Surface::clear(const Color &color)
            {
                if (!bIsDrawing)
                    return;

                ::glClearColor(color.red(), color.green(), color.blue(), color.alpha());
                ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//                const float w = 0.25f * nWidth;
//                const float h = 0.25f * nHeight;
//
//                glDisable(GL_MULTISAMPLE);
//                glBegin(GL_TRIANGLES);
//                    glColor4f(1.0f, 0.0f, 0.0f, 0.0f);
//                    glVertex2f(w * 2, h * 2);
//                    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
//                    glVertex2f(w, 0);
//                    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
//                    glVertex2f(0, h*2);
//                glEnd();
//                glEnable(GL_MULTISAMPLE);
//                glBegin(GL_TRIANGLES);
//                    glColor4f(1.0f, 0.0f, 0.0f, 0.0f);
//                    glVertex2f(w * 4, h * 4);
//                    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
//                    glVertex2f(w * 3, h * 2);
//                    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
//                    glVertex2f(w * 2, h * 4);
//                glEnd();
            }

            void Surface::wire_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // TODO
            }

            void Surface::wire_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // TODO
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                // TODO
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                // TODO
            }

            void Surface::fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
            {
                // TODO
            }

            void Surface::fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // TODO
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                // TODO
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                // TODO
            }

            void Surface::fill_sector(const Color &c, float x, float y, float r, float a1, float a2)
            {
                if (!bIsDrawing)
                    return;

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

                // Start batch
                status_t res = sBatch.begin(gl::batch_header_t {
                    gl::SIMPLE,
                    bAntiAliasing,
                });
                if (res != STATUS_OK)
                    return;
                lsp_finally { sBatch.end(); };

                float vx        = cosf(a1) * r;
                float vy        = sinf(a1) * r;

                const float ci      = sBatch.color(c.red(), c.green(), c.blue(), c.alpha());
                const size_t v0i    = sBatch.vertex(x, y, ci);
                size_t v1i          = sBatch.vertex(x + vx, y + vy, ci);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(x + vx, y + vy, ci);
                    sBatch.triangle(v0i, v1i, v1i + 1);
                    ++v1i;
                }

                sBatch.vertex(x + ex, y + ey, ci);
                sBatch.triangle(v0i, v1i, v1i + 1);
            }

            void Surface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // TODO
            }

            void Surface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (!bIsDrawing)
                    return;

                // Start batch
                status_t res = sBatch.begin(gl::batch_header_t {
                    gl::SIMPLE,
                    bAntiAliasing,
                });
                if (res != STATUS_OK)
                    return;
                lsp_finally { sBatch.end(); };

                // Fill batch with single triangle
                {
                    const float ci      = sBatch.color(c.red(), c.green(), c.blue(), c.alpha());
                    const ssize_t vi    = sBatch.vertex(x0, y0, ci);
                    sBatch.vertex(x1, y1, ci);
                    sBatch.vertex(x2, y2, ci);
                    sBatch.triangle(vi, vi+1, vi+2);
                }

//                // TODO: remove this code after batch execution implementation complete
//                {
//                    glColor4f(c.red(), c.green(), c.blue(), c.alpha());
//                    glBegin(GL_TRIANGLES);
//                        glVertex2f(x0, y0);
//                        glVertex2f(x1, y1);
//                        glVertex2f(x2, y2);
//                    glEnd();
//                }
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
                if (!bIsDrawing)
                    return;

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

                // Start batch
                status_t res = sBatch.begin(gl::batch_header_t {
                    gl::SIMPLE,
                    bAntiAliasing,
                });
                if (res != STATUS_OK)
                    return;
                lsp_finally { sBatch.end(); };

                // Fill batch
                float vx        = cosf(a1) * ro;
                float vy        = sinf(a1) * ro;

                const float ci      = sBatch.color(c.red(), c.green(), c.blue(), c.alpha());
                ssize_t v0i         = sBatch.vertex(x + vx * kr, y + vy * kr, ci);
                sBatch.vertex(x + vx, y + vy, ci);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(x + vx * kr, y + vy * kr, ci);
                    sBatch.vertex(x + vx, y + vy, ci);
                    sBatch.rectangle(v0i, v0i + 1, v0i + 3, v0i + 2);
//                    sBatch.triangle(v0i, v0i + 1, v0i + 2);
//                    sBatch.triangle(v0i + 2, v0i + 1, v0i + 3);
                    v0i        += 2;
                }

                sBatch.vertex(x + ex * kr, y + ey * kr, ci);
                sBatch.vertex(x + ex, y + ey, ci);
                sBatch.rectangle(v0i, v0i + 1, v0i + 3, v0i + 2);
//                sBatch.triangle(v0i, v0i + 1, v0i + 2);
//                sBatch.triangle(v0i + 2, v0i + 1, v0i + 3);
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
                if (!bIsDrawing)
                    return;

                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float alpha = lsp_min(4.0f / r, M_PI_2);
                const float dx = cosf(alpha);
                const float dy = sinf(alpha);
                const size_t count = M_PI * 0.5f * r;

                // Start batch
                status_t res = sBatch.begin(gl::batch_header_t {
                    gl::SIMPLE,
                    bAntiAliasing,
                });
                if (res != STATUS_OK)
                    return;
                lsp_finally { sBatch.end(); };

                // Fill batch
                float vx = r;
                float vy = 0.0f;

                const float ci      = sBatch.color(c.red(), c.green(), c.blue(), c.alpha());
                const ssize_t v0i   = sBatch.vertex(x, y, ci);
                size_t v1i          = sBatch.vertex(x + vx, y + vy, ci);

                for (size_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    sBatch.vertex(x + vx, y + vy, ci);
                    sBatch.triangle(v0i, v1i, v1i + 1);
                    ++v1i;
                }

                sBatch.vertex(x + r, y, ci);
                sBatch.triangle(v0i, v1i, v1i + 1);
            }

            void Surface::fill_circle(IGradient *g, float x, float y, float r)
            {
            }

            void Surface::fill_frame(
                const Color &color,
                size_t flags, float radius,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                // TODO
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
            #ifdef LSP_DEBUG
                ++nNumClips;
            #endif /* LSP_DEBUG */
            }

            void Surface::clip_end()
            {
            #ifdef LSP_DEBUG
                if (nNumClips <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                -- nNumClips;
            #endif /* LSP_DEBUG */
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(USE_LIBX11) */

