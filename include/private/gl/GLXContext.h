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

#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/IContext.h>
#include <private/gl/glx_vtbl.h>

#include <GL/gl.h>
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
                    enum pflags_t
                    {
                        PF_VERTEX       = 1 << 0,
                        PF_FRAGMENT     = 1 << 1,
                        PF_PROGRAM      = 1 << 2,
                    };

                    enum compile_status_t
                    {
                        SHADER,
                        PROGRAM
                    };

                    typedef struct program_t
                    {
                        GLuint          nVertexId;
                        GLuint          nFragmentId;
                        GLuint          nProgramId;
                        uint32_t        nFlags;
                    } program_t;

                private:
                    ::Display          *pDisplay;
                    ::GLXContext        hContext;
                    ::Window            hWindow;
                    const vtbl_t       *pVtbl;

                    lltl::parray<program_t> vPrograms;

                private:
                    static const char  *vertex_shader(size_t program_id);
                    static const char  *fragment_shader(size_t program_id);
                    static bool         check_gl_error(const char *context);

                private:
                    void                destroy(program_t *prg);
                    bool                check_compile_status(const char *context, GLenum id, compile_status_t type);

                protected:
                    virtual status_t    do_activate() override;
                    virtual status_t    do_deactivate() override;

                public:
                    explicit Context(::Display *dpy, ::GLXContext ctx, ::Window wnd, vtbl_t *vtbl);
                    virtual ~Context() override;

                public:
                    virtual status_t    program(size_t *id, gl::program_t program) override;
            };

            /**
             * Create GLX context
             * @param dpy display
             * @param screen screen
             * @param window associated window
             * @return pointer to created context or NULL
             */
            gl::IContext *create_context(Display *dpy, int screen, Window window);

        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */

#endif /* PRIVATE_GL_GLXCONTEXT_H_ */
