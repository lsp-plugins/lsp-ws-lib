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
#include <private/gl/Surface.h>
#include <private/x11/X11Display.h>

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

            Surface::~Surface()
            {
                // TODO
            }

            void Surface::destroy()
            {
                // TODO
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
                // Force end() call
                end();

                // TODO

            #ifdef LSP_DEBUG
                nNumClips       = 0;
            #endif /* LSP_DEBUG */
            }

            void Surface::end()
            {
                // TODO

            #ifdef LSP_DEBUG
                if (nNumClips > 0)
                    lsp_error("Mismatching number of clip_begin() and clip_end() calls");
            #endif /* LSP_DEBUG */
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
                // TODO
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
                // TODO
            }

            void Surface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // TODO
            }

            void Surface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // TODO
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
                // TODO
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
                // TODO
            }

            void Surface::fill_circle(IGradient *g, float x, float y, float r)
            {
                // TODO
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
                // TODO
                return false;
            }

            bool Surface::set_antialiasing(bool set)
            {
                // TODO
                return false;
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

