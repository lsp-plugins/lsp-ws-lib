/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 янв. 2025 г.
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

#include <private/x11/X11GLSurface.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            X11GLSurface::X11GLSurface(X11Display *display, gl::IContext *ctx, size_t width, size_t height):
                gl::Surface(display, ctx, width, height)
            {
                pX11Display = display;
            }

            X11GLSurface::X11GLSurface(X11Display *display, size_t width, size_t height):
                gl::Surface(width, height)
            {
                pX11Display = display;
            }

            X11GLSurface::~X11GLSurface()
            {
            }

            gl::Surface *X11GLSurface::create_nested(size_t width, size_t height)
            {
                return new X11GLSurface(pX11Display, width, height);
            }

            bool X11GLSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                return false;
            }

            bool X11GLSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                return false;
            }

            bool X11GLSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                return false;
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
            }

            void X11GLSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
            }

            void X11GLSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
            }

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */



