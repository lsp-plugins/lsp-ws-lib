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

        #ifdef TRACE_OPENGL_ALLOCATIONS
            inline void trace_array(const char *func, lltl::darray<GLuint> & list)
            {
                for (size_t i=0, n=list.size(); i<n; ++i)
                {
                    GLuint *id = list.uget(i);
                    if (id != NULL)
                        lsp_trace("%s(%d)", func, int(*id));
                }
            }

            inline void trace_alloc(const char *func, GLuint id)
            {
                lsp_trace("%s(%d)", func, int(id));
            }
        #else
            #define trace_array(...)
            #define trace_alloc(...)
        #endif

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
                bValid              = true;
                pVtbl               = vtbl;

                nCommandsId         = 0;
                nCommandsSize       = 0;
                nCommandsProcessor  = GL_NONE;
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
                const size_t n = ids.size(), m = list.size();
                const GLuint *src = dst, *lst = list.first();
                const GLuint *esrc = &src[n], *elst = &lst[m];

                // Remove elements that meet lst
                while (src < esrc)
                {
                    // Check that current element matches
                    if (*src == *lst)
                    {
                        ++src;
                        if ((++lst) >= elst)
                            break;
                    }
                    else
                    {
                        // Copy data if needed
                        if (dst != src)
                            *dst    = *src;
                        ++dst;
                        ++src;
                    }
                }

                // Move tail
                if (dst != src)
                {
                    while (src < esrc)
                        *(dst++)  = *(src++);
                }

                // Evict removed elements from the tail
                ids.pop_n(src - dst);

            #ifdef TRACE_OPENGL_ALLOCATIONS
                lsp_trace("Size: %d, requested: %d, removed: %d, active: %d",
                    int(n), int(m), int(src - dst), int(ids.size()));
            #endif /* TRACE_OPENGL_ALLOCATIONS */
            }

            void IContext::perform_gc()
            {
                if (vGcFramebuffers.size() > 0)
                {
                    trace_array("glDeleteFramebuffers", vGcFramebuffers);
                    pVtbl->glDeleteFramebuffers(vGcFramebuffers.size(), vGcFramebuffers.first());
                    remove_identifiers(vFramebuffers, vGcFramebuffers);
                }
                if (vGcRenderbuffers.size() > 0)
                {
                    trace_array("glDeleteRenderbuffers", vGcRenderbuffers);
                    pVtbl->glDeleteRenderbuffers(vGcRenderbuffers.size(), vGcRenderbuffers.first());
                    remove_identifiers(vRenderbuffers, vGcRenderbuffers);
                }
                if (vGcTextures.size() > 0)
                {
                    trace_array("glDeleteTextures", vGcTextures);
                    pVtbl->glDeleteTextures(vGcTextures.size(), vGcTextures.first());
                    remove_identifiers(vTextures, vGcTextures);
                }
            }

            void IContext::cleanup()
            {
                // Flush GC buffers as they are not needed
                vGcFramebuffers.flush();
                vGcRenderbuffers.flush();
                vGcTextures.flush();

                // Free all framebuffers
                if (vFramebuffers.size() > 0)
                {
                    trace_array("glDeleteFramebuffers", vFramebuffers);
                    pVtbl->glDeleteFramebuffers(vFramebuffers.size(), vFramebuffers.first());
                    vFramebuffers.flush();
                }

                // Free all renderbuffers
                if (vRenderbuffers.size() > 0)
                {
                    trace_array("glDeleteRenderbuffers", vRenderbuffers);
                    pVtbl->glDeleteRenderbuffers(vRenderbuffers.size(), vRenderbuffers.first());
                    vRenderbuffers.flush();
                }

                // Free all textures
                if (vTextures.size() > 0)
                {
                    trace_array("glDeleteTextures", vTextures);
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

            GLint IContext::attribute_location(program_t program, attribute_t attribute)
            {
                return -STATUS_NOT_FOUND;
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
                    trace_alloc("glGenFramebuffers", id);
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
                    trace_alloc("glGenRenderbuffers", id);
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
                    trace_alloc("glGenTextures", id);
                    *dst_id         = id;
                    return id;
                }

                // Deallocate texture on error
                pVtbl->glDeleteTextures(1, &id);
                return 0;
            }

            void IContext::free_framebuffer(GLuint id)
            {
                if (bValid)
                    vGcFramebuffers.add(&id);
            }

            void IContext::free_renderbuffer(GLuint id)
            {
                if (bValid)
                    vGcRenderbuffers.add(&id);
            }

            void IContext::free_texture(GLuint id)
            {
                if (bValid)
                    vGcTextures.add(&id);
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

            status_t IContext::load_command_buffer(const float *buf, size_t size, size_t length)
            {
                // Need to allocate new texture?
                if (nCommandsId == 0)
                {
                    if ((nCommandsId = alloc_texture()) == 0)
                        return STATUS_NO_MEM;
                }

                // Initialize and bind texture
                pVtbl->glActiveTexture(GL_TEXTURE0);
                pVtbl->glBindTexture(GL_TEXTURE_2D, nCommandsId);

                // Load full texture if size has changed or partial texture if size didn't change
                if (nCommandsSize != size)
                {
                    pVtbl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size, size, 0, GL_RGBA, GL_FLOAT, buf);
                    nCommandsSize   = size;
                }
                else
                {
                    const size_t stride = size * 4;
                    const size_t rows = (length + stride - 1) / stride;
                    pVtbl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, rows, GL_RGBA, GL_FLOAT, buf);
                }

                // Unbind texture
                pVtbl->glBindTexture(GL_TEXTURE_2D, 0);

                return STATUS_OK;
            }

            status_t IContext::bind_command_buffer(GLuint processor_id)
            {
                if (nCommandsId == 0)
                    return STATUS_BAD_STATE;
                if (nCommandsProcessor != GL_NONE)
                    return STATUS_ALREADY_BOUND;

                // Initialize and bind texture
                pVtbl->glActiveTexture(processor_id);
                pVtbl->glBindTexture(GL_TEXTURE_2D, nCommandsId);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                return STATUS_OK;
            }

            void IContext::unbind_command_buffer()
            {
                if (nCommandsProcessor == GL_NONE)
                    return;

                // Initialize and bind texture
                pVtbl->glActiveTexture(nCommandsProcessor);
                pVtbl->glBindTexture(GL_TEXTURE_2D, 0);
                nCommandsProcessor          = GL_NONE;
            }

            status_t IContext::bind_empty_texture(GLuint processor_id, size_t samples)
            {
                pVtbl->glActiveTexture(processor_id);

                const GLuint tex_kind = (samples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
                GLuint texture_id = GL_NONE;

                // Lookup matching texture
                for (size_t i=0, n=vEmpty.size(); i<n; ++i)
                {
                    texture_t *t = vEmpty.uget(i);
                    if (t->nSamples == samples)
                    {
                        texture_id      = t->nId;
                        break;
                    }
                }

                // Allocate and initialize texture if not found
                if (texture_id == GL_NONE)
                {
                    // Allocate texture
                    if ((texture_id = alloc_texture()) == GL_NONE)
                        return STATUS_NO_MEM;

                    // Register texture
                    texture_t *t = vEmpty.add();
                    if (t == NULL)
                    {
                        free_texture(texture_id);
                        return STATUS_NO_MEM;
                    }
                    t->nId          = texture_id;
                    t->nSamples     = samples;

                    // Bind and initialize texture
                    pVtbl->glBindTexture(tex_kind, texture_id);
                    if (samples > 0)
                        pVtbl->glTexImage2DMultisample(tex_kind, samples, GL_RGBA, 1, 1, GL_TRUE);
                    else
                        pVtbl->glTexImage2D(tex_kind, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                }
                else
                    pVtbl->glBindTexture(tex_kind, texture_id);

                // Set texture parameters
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                pVtbl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                return STATUS_OK;
            }

            void IContext::unbind_empty_texture(GLuint processor_id, size_t samples)
            {
                const GLuint tex_kind = (samples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
                pVtbl->glBindTexture(tex_kind, GL_NONE);
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

