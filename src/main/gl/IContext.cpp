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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <private/gl/IContext.h>
#include <private/glx/Context.h>

#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            IContext::IContext()
            {
                atomic_store(&nReferences, 1);
                bValid      = true;
            }

            IContext::~IContext()
            {
            }

            uatomic_t IContext::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t IContext::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                    delete this;
                return result;
            }

            void IContext::perform_gc()
            {
                const vtbl_t *vtbl  = this->vtbl();
                if (vGcFramebuffer.size() > 0)
                {
//                    for (size_t i=0; i<vGcFramebuffer.size(); ++i)
//                    {
//                        GLuint *id = vGcFramebuffer.uget(i);
//                        lsp_trace("glDeleteFramebuffers(%d)", int(*id));
//                    }
                    vtbl->glDeleteFramebuffers(vGcFramebuffer.size(), vGcFramebuffer.first());
                    vGcFramebuffer.clear();
                }
                if (vGcRenderbuffer.size() > 0)
                {
//                    for (size_t i=0; i<vGcRenderbuffer.size(); ++i)
//                    {
//                        GLuint *id = vGcRenderbuffer.uget(i);
//                        lsp_trace("glDeleteRenderbuffers(%d)", int(*id));
//                    }
                    vtbl->glDeleteRenderbuffers(vGcRenderbuffer.size(), vGcRenderbuffer.first());
                    vGcRenderbuffer.clear();
                }
                if (vGcTexture.size() > 0)
                {
//                    for (size_t i=0; i<vGcTexture.size(); ++i)
//                    {
//                        GLuint *id = vGcTexture.uget(i);
//                        lsp_trace("glDeleteTextures(%d)", int(*id));
//                    }
                    vtbl->glDeleteTextures(vGcTexture.size(), vGcTexture.first());
                    vGcTexture.clear();
                }
            }

            bool IContext::active() const
            {
                return false;
            }

            status_t IContext::activate()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t IContext::deactivate()
            {
                perform_gc();
                return STATUS_NOT_IMPLEMENTED;
            }

            void IContext::swap_buffers(size_t width, size_t height)
            {
            }

            void IContext::invalidate()
            {
                activate();
                perform_gc();
                deactivate();

                lsp_trace("OpenGL context invalidated ptr=%p", this);
                bValid          = false;
            }

            bool IContext::valid() const
            {
                return bValid;
            }

            status_t IContext::program(size_t *id, program_t program)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const vtbl_t *IContext::vtbl() const
            {
                return NULL;
            }

            uint32_t IContext::multisample() const
            {
                return 0;
            }

            void IContext::free_framebuffer(GLuint id)
            {
                vGcFramebuffer.add(&id);
            }

            void IContext::free_renderbuffer(GLuint id)
            {
                vGcRenderbuffer.add(&id);
            }

            void IContext::free_texture(GLuint id)
            {
                vGcTexture.add(&id);
            }

            gl::IContext *create_context(const context_param_t *params)
            {
                const context_param_t *display = NULL;
                const context_param_t *screen = NULL;
                const context_param_t *window = NULL;

                for (; (params != NULL) && (params->id != gl::END); ++params)
                {
                    switch (params->id)
                    {
                        case gl::DISPLAY:
                            display     = params;
                            break;
                        case gl::SCREEN:
                            screen      = params;
                            break;
                        case gl::WINDOW:
                            window      = params;
                            break;
                        default:
                            return NULL;
                    }
                }

                gl::IContext *result = NULL;

            #ifdef LSP_PLUGINS_USE_OPENGL_GLX
                if ((result == NULL) && (display != NULL) && (window != NULL))
                {
                    ::Display *dpy = static_cast<::Display *>(display->ptr);
                    ::Window wnd = window->ulong;
                    const int scr = (screen != NULL) ? screen->sint : DefaultScreen(dpy);

                    result = glx::create_context(dpy, scr, wnd);
                }
            #endif /* LSP_PLUGINS_USE_OPENGL_GLX */

                return result;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

