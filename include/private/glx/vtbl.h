/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 19 янв. 2025 г.
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

#ifndef PRIVATE_GL_GLX_VTBL_H_
#define PRIVATE_GL_GLX_VTBL_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL_GLX

#include <private/gl/vtbl.h>

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            typedef void (*procaddress_t)();

            typedef struct vtbl_t: public gl::vtbl_t
            {
                procaddress_t (* glXGetProcAddress)(const GLubyte * procName);
                GLXContext (* glXCreateContextAttribsARB)(
                    Display *dpy,
                    GLXFBConfig config,
                    GLXContext share_context,
                    Bool direct,
                    const int *attrib_list);
            } vtbl_t;

            /**
             * Create virtual table with OpenGL functions related to GLX
             * @return virtual table with OpenGL functions related to GLX
             */
            glx::vtbl_t *create_vtbl();
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */


#endif /* LSP_PLUGINS_USE_OPENGL_GLX */

#endif /* INCLUDE_PRIVATE_GL_GLX_VTBL_H_ */
