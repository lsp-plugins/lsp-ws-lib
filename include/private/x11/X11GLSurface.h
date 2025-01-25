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

#ifndef PRIVATE_X11_X11GLSURFACE_H_
#define PRIVATE_X11_X11GLSURFACE_H_

#include <lsp-plug.in/ws/version.h>

#include <private/x11/X11Display.h>
#include <private/gl/Surface.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            /**
             * OpenGL rendering surface for X11 implementation
             */
            class LSP_HIDDEN_MODIFIER X11GLSurface: public gl::Surface
            {
                private:
                    X11Display        *pX11Display;

                public:
                    /** Create GL surface
                     *
                     * @param display associated display
                     * @param ctx OpenGL context
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11GLSurface(X11Display *display, gl::IContext *ctx, size_t width, size_t height);

                    X11GLSurface(const X11GLSurface &) = delete;
                    X11GLSurface(X11GLSurface &&) = delete;
                    virtual ~X11GLSurface() override;

                    X11GLSurface & operator = (const X11GLSurface &) = delete;
                    X11GLSurface & operator = (X11GLSurface &&) = delete;

                protected:
                    /** Create nested GL surface
                     *
                     * @param display display
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11GLSurface(X11Display *display, size_t width, size_t height);

                    /**
                     * Factory method for creating nested surface with proper class type
                     * @param width width of the nested surface
                     * @param height heigth of the nested surface
                     * @return pointer to created surface
                     */
                    virtual gl::Surface *create_nested(size_t width, size_t height) override;

                public: // ws::ISurface implementation
                    virtual bool get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const char *text) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last) override;
            };

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PRIVATE_X11_X11GLSURFACE_H_ */
