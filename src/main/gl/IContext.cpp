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
            static ssize_t cmp_gluint(const GLuint *a, const GLuint *b)
            {
                const size_t ia = *a;
                const size_t ib = *b;

            #ifdef ARCH_32_BIT
                return (ia < ib) ? -1 : (ia > ib) ? 1 : 0
            #else
                return ssize_t(ia - ib);
            #endif /* ARCH_32_BIT */
            }

            IContext::IContext(const gl::vtbl_t *vtbl)
            {
                atomic_store(&nReferences, 1);
                bValid      = true;
                pVtbl       = vtbl;
            }

            IContext::~IContext()
            {
                // Destroy virtual table
                if (pVtbl != NULL)
                {
                    free(const_cast<vtbl_t *>(pVtbl));
                    pVtbl           = NULL;
                }
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

            void IContext::remove_identifiers(lltl::darray<GLuint> & ids, lltl::darray<GLuint> & list)
            {
                // Check that removal list is empty
                if (list.is_empty())
                    return;
                lsp_finally { list.clear(); };

                // Sort identifiers in identifier list and removal list
                ids.qsort(cmp_gluint);
                list.qsort(cmp_gluint);

                GLuint *dst = ids.first();
                size_t i = 0, j = 0;
                const GLuint *src = dst, *lst = list.first();
                const size_t n = ids.size(), m = list.size();

                // Remove elements that meet lst
                for ( ; i < n; ++i, ++src)
                {
                    // Check that current element matches
                    if (*src == *lst)
                    {
                        if ((++j) >= m)
                            break;
                        ++lst;
                    }
                    else if (dst != src)
                        *(dst++)  = *src;
                }

                // Move tail
                if (dst != src)
                {
                    for ( ; i < n; ++i, ++src)
                        *(dst++)  = *src;
                }

                // Evict removed elements from the tail
                ids.pop_n(src - dst);
                lsp_trace("Removed %d identifiers, %d identifiers active", int(src - dst), int(ids.size()));
            }

            void IContext::perform_gc()
            {
                if (vGcFramebuffer.size() > 0)
                {
                    for (size_t i=0; i<vGcFramebuffer.size(); ++i)
                    {
                        GLuint *id = vGcFramebuffer.uget(i);
                        lsp_trace("glDeleteFramebuffers(%d)", int(*id));
                    }
                    pVtbl->glDeleteFramebuffers(vGcFramebuffer.size(), vGcFramebuffer.first());
                    remove_identifiers(vFramebuffers, vGcFramebuffer);
                }
                if (vGcRenderbuffer.size() > 0)
                {
                    for (size_t i=0; i<vGcRenderbuffer.size(); ++i)
                    {
                        GLuint *id = vGcRenderbuffer.uget(i);
                        lsp_trace("glDeleteRenderbuffers(%d)", int(*id));
                    }
                    pVtbl->glDeleteRenderbuffers(vGcRenderbuffer.size(), vGcRenderbuffer.first());
                    remove_identifiers(vRenderbuffers, vGcRenderbuffer);
                }
                if (vGcTexture.size() > 0)
                {
                    for (size_t i=0; i<vGcTexture.size(); ++i)
                    {
                        GLuint *id = vGcTexture.uget(i);
                        lsp_trace("glDeleteTextures(%d)", int(*id));
                    }
                    pVtbl->glDeleteTextures(vGcTexture.size(), vGcTexture.first());
                    remove_identifiers(vTextures, vGcTexture);
                }
            }

            void IContext::cleanup()
            {
                if (vFramebuffers.size() > 0)
                {
                    for (size_t i=0; i<vFramebuffers.size(); ++i)
                    {
                        GLuint *id = vFramebuffers.uget(i);
                        lsp_trace("glDeleteFramebuffers(%d)", int(*id));
                    }
                    pVtbl->glDeleteFramebuffers(vFramebuffers.size(), vFramebuffers.first());
                    vFramebuffers.flush();
                }
                if (vRenderbuffers.size() > 0)
                {
                    for (size_t i=0; i<vRenderbuffers.size(); ++i)
                    {
                        GLuint *id = vRenderbuffers.uget(i);
                        lsp_trace("glDeleteRenderbuffers(%d)", int(*id));
                    }
                    pVtbl->glDeleteRenderbuffers(vRenderbuffers.size(), vRenderbuffers.first());
                    vRenderbuffers.flush();
                }
                if (vTextures.size() > 0)
                {
                    for (size_t i=0; i<vTextures.size(); ++i)
                    {
                        GLuint *id = vTextures.uget(i);
                        lsp_trace("glDeleteTextures(%d)", int(*id));
                    }
                    pVtbl->glDeleteTextures(vTextures.size(), vTextures.first());
                    vTextures.flush();
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
                if (activate() == STATUS_OK)
                {
                    lsp_finally { deactivate(); };
                    cleanup();
                }

                lsp_trace("OpenGL context invalidated ptr=%p", this);
                bValid          = false;
            }

            status_t IContext::program(size_t *id, program_t program)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            uint32_t IContext::multisample() const
            {
                return 0;
            }

            size_t IContext::width() const
            {
                return 0;
            }

            size_t IContext::height() const
            {
                return 0;
            }

            GLuint IContext::alloc_framebuffer()
            {
                if (activate() != STATUS_OK)
                    return 0;

                // Allocate framebuffer
                GLuint id = 0;
                pVtbl->glGenFramebuffers(1, &id);
                if (id == 0)
                    return id;

                // Register framebuffer identifier
                GLuint *dst_id  = vFramebuffers.add();
                if (dst_id != NULL)
                {
                    *dst_id         = id;
                    return id;
                }

                // Deallocate framebuffer on error
                pVtbl->glDeleteFramebuffers(1, &id);
                return 0;
            }

            GLuint IContext::alloc_renderbuffer()
            {
                if (activate() != STATUS_OK)
                    return 0;

                // Allocate texture
                GLuint id = 0;
                pVtbl->glGenRenderbuffers(1, &id);
                if (id == 0)
                    return id;

                // Register texture identifier
                GLuint *dst_id  = vRenderbuffers.add();
                if (dst_id != NULL)
                {
                    *dst_id         = id;
                    return id;
                }

                // Deallocate texture on error
                pVtbl->glDeleteRenderbuffers(1, &id);
                return 0;
            }

            GLuint IContext::alloc_texture()
            {
                if (activate() != STATUS_OK)
                    return 0;

                // Allocate texture
                GLuint id = 0;
                pVtbl->glGenTextures(1, &id);
                if (id == 0)
                    return id;

                // Register texture identifier
                GLuint *dst_id  = vTextures.add();
                if (dst_id != NULL)
                {
                    *dst_id         = id;
                    return id;
                }

                // Deallocate texture on error
                pVtbl->glDeleteTextures(1, &id);
                return 0;
            }

            void IContext::free_framebuffer(GLuint id)
            {
                if (valid())
                    vGcFramebuffer.add(&id);
            }

            void IContext::free_renderbuffer(GLuint id)
            {
                if (valid())
                    vGcRenderbuffer.add(&id);
            }

            void IContext::free_texture(GLuint id)
            {
                if (valid())
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

