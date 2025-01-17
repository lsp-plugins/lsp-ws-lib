/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 янв. 2025 г.
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

#ifndef PRIVATE_GL_GLXCONTEXT_H_
#define PRIVATE_GL_GLXCONTEXT_H_

#include <lsp-plug.in/common/types.h>

#if defined(USE_LIBX11)

#include <private/gl/IContext.h>

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            /**
             * GLX context
             */
            class LSP_HIDDEN_MODIFIER Context: public gl::IContext
            {
                private:
                    GLXContext        hContext;

                public:
                    explicit Context(GLXContext ctx);
                    virtual ~Context() override;

                protected:
                    virtual status_t    do_activate() override;
                    virtual status_t    do_deactivate() override;
                    virtual const char *shader(gl::shader_t shader) const override;
            };

            gl::IContext *create_context(Display *dpy);

        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */

#endif /* PRIVATE_GL_GLXCONTEXT_H_ */
