/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 23 янв. 2025 г.
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

#include <private/gl/Texture.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            Texture::Texture(gl::IContext *ctx)
            {
                pContext            = safe_acquire(ctx);
                atomic_store(&nReferences, 1);
                nTextureId          = 0;
                nFrameBufferId      = 0;
                nStencilBufferId    = 0;
                for (size_t i=0; i<MAX_PROCESSOR_IDS; ++i)
                    vProcessorIds[i]    = 0;
                nProcessorIds       = 0;
                nWidth              = 0;
                nHeight             = 0;
                enFormat            = gl::TEXTURE_UNKNOWN;
                nSamples            = 0;
            }

            Texture::~Texture()
            {
                if (nTextureId != 0)
                    lsp_trace("this=%p, id=%d", this, int(nTextureId));
                reset();
            }

            uatomic_t Texture::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t Texture::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                {
                    reset();
                    delete this;
                }
                return result;
            }

            GLuint Texture::allocate_texture()
            {
                if (nTextureId != 0)
                    return nTextureId;

                nTextureId = pContext->alloc_texture();
//                lsp_trace("glGenTextures(%d)", int(nTextureId));

                return nTextureId;
            }

            GLuint Texture::allocate_framebuffer()
            {
                if (nFrameBufferId != 0)
                    return nFrameBufferId;

                nFrameBufferId = pContext->alloc_framebuffer();
//                lsp_trace("glGenFramebuffers(%d)", int(nFrameBufferId));

                return nFrameBufferId;
            }

            GLuint Texture::allocate_stencil()
            {
                if (nStencilBufferId != 0)
                    return nStencilBufferId;

                nStencilBufferId = pContext->alloc_renderbuffer();
//                lsp_trace("glGenRenderbuffers(%d)", int(nStencilBufferId));

                return nStencilBufferId;
            }

            status_t Texture::set_image(const void *buf, size_t width, size_t height, size_t stride, texture_format_t format)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if (format == gl::TEXTURE_UNKNOWN)
                    return STATUS_INVALID_VALUE;

                // Deallocate related buffers if they have been previously set
                deallocate_buffers();

                // Initialize texture
                const vtbl_t *vtbl = pContext->vtbl();
                const size_t pixel_size = (format == gl::TEXTURE_ALPHA8) ? sizeof(uint8_t) : sizeof(uint32_t);
                const GLuint tex_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_BGRA;
                const GLuint int_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                const GLuint texture_id = allocate_texture();
                if (texture_id == 0)
                    return STATUS_NO_MEM;

                // Load texture contents
                const GLuint num_of_pixels = stride / pixel_size;
                if (num_of_pixels != width)
                    vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, num_of_pixels);

                vtbl->glBindTexture(GL_TEXTURE_2D, texture_id);
                vtbl->glTexImage2D(GL_TEXTURE_2D, 0, int_format, width, height, 0, tex_format, GL_UNSIGNED_BYTE, buf);
                vtbl->glBindTexture(GL_TEXTURE_2D, GL_NONE);

                if (num_of_pixels != width)
                    vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                // Update texture settings
                nWidth      = uint32_t(width);
                nHeight     = uint32_t(height);
                enFormat    = format;
                nSamples    = 0;

                return STATUS_OK;
            }

            status_t Texture::resize(size_t width, size_t height)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if ((enFormat == gl::TEXTURE_UNKNOWN) || (nTextureId == 0) || (nSamples > 0))
                    return STATUS_OK;
                if ((nWidth == width) && (nHeight == height))
                    return STATUS_OK;

                // Activate context
                status_t res = pContext->activate();
                if (res != STATUS_OK)
                    return res;

                // Resize texture
                const vtbl_t *vtbl = pContext->vtbl();
                const GLuint tex_format = (enFormat == gl::TEXTURE_ALPHA8) ? GL_RED : GL_BGRA;
                const GLuint int_format = (enFormat == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                vtbl->glBindTexture(GL_TEXTURE_2D, nTextureId);
                vtbl->glTexImage2D(GL_TEXTURE_2D, 0, int_format, width, height, 0, tex_format, GL_UNSIGNED_BYTE, NULL);
                vtbl->glBindTexture(GL_TEXTURE_2D, GL_NONE);

                // Update texture settings
                nWidth      = uint32_t(width);
                nHeight     = uint32_t(height);

                return STATUS_OK;
            }

            status_t Texture::set_subimage(const void *buf, size_t x, size_t y, size_t width, size_t height, size_t stride)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if (enFormat == gl::TEXTURE_UNKNOWN)
                    return STATUS_BAD_STATE;
                if (nTextureId == 0)
                    return STATUS_BAD_STATE;

                const vtbl_t *vtbl = pContext->vtbl();
                const size_t pixel_size = (enFormat == gl::TEXTURE_ALPHA8) ? sizeof(uint8_t) : sizeof(uint32_t);
                const GLuint tex_format = (enFormat == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / pixel_size);
                lsp_finally { vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); };

                if (vtbl->glTextureSubImage2D)
                    vtbl->glTextureSubImage2D(nTextureId, 0, x, y, width, height, tex_format, GL_UNSIGNED_BYTE, buf);
                else
                {
                    vtbl->glBindTexture(GL_TEXTURE_2D, nTextureId);
                    vtbl->glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, tex_format, GL_UNSIGNED_BYTE, buf);
                    vtbl->glBindTexture(GL_TEXTURE_2D, GL_NONE);
                }

                nSamples    = 0;

                return STATUS_OK;
            }

            bool Texture::bind_processor(GLuint processor_id)
            {
                if (nProcessorIds >= MAX_PROCESSOR_IDS)
                    return false;

                // Check that we do not contain specified processor
                for (size_t i=0; i<nProcessorIds; ++i)
                    if (vProcessorIds[i] == processor_id)
                        return false;

                vProcessorIds[nProcessorIds++] = processor_id;
                return true;
            }

            bool Texture::unbind_processor(GLuint processor_id)
            {
                if (nProcessorIds <= 0)
                    return false;

                GLuint *dst = vProcessorIds;
                GLuint *end = &vProcessorIds[nProcessorIds];

                // Find matching processor
                for ( ; dst < end; ++dst)
                    if (*dst == processor_id)
                        break;

                if (dst == end)
                    return false;

                // Move data
                for ( ++dst; dst < end; ++dst)
                    dst[-1]     = dst[0];
                dst[-1]     = 0;

                --nProcessorIds;

                return true;
            }

            void Texture::bind(GLuint processor_id)
            {
                if (pContext == NULL)
                    return;

                if (!bind_processor(processor_id))
                {
                    lsp_warn("Error binding texture %p to processor %d", this, int(processor_id));
                    return;
                }

                const vtbl_t *vtbl = pContext->vtbl();

                const GLenum tex_kind = (nSamples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

                vtbl->glActiveTexture(processor_id);
                vtbl->glBindTexture(tex_kind, nTextureId);
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            void Texture::unbind(GLuint processor_id)
            {
                if (pContext == NULL)
                    return;

                if (!unbind_processor(processor_id))
                {
                    lsp_warn("Error unbinding texture %p to processor %d", this, int(processor_id));
                    return;
                }

                const vtbl_t *vtbl = pContext->vtbl();

                const GLenum tex_kind = (nSamples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
                vtbl->glActiveTexture(processor_id);
                vtbl->glBindTexture(tex_kind, GL_NONE);
            }

            void Texture::deallocate_buffers()
            {
                if (nFrameBufferId != 0)
                {
                    pContext->free_framebuffer(nFrameBufferId);
                    nFrameBufferId      = 0;
                }
                if (nStencilBufferId != 0)
                {
                    pContext->free_renderbuffer(nStencilBufferId);
                    nStencilBufferId    = 0;
                }
            }

            void Texture::reset()
            {
                if (pContext == NULL)
                    return;

                deallocate_buffers();

                if (nTextureId != 0)
                {
                    pContext->free_texture(nTextureId);
                    nTextureId      = 0;
                }

                safe_release(pContext);
                nSamples          = 0;
            }

            size_t Texture::size() const
            {
                if (enFormat == gl::TEXTURE_UNKNOWN)
                    return 0;

                const size_t szof = (enFormat == gl::TEXTURE_ALPHA8) ? sizeof(uint8_t) : sizeof(uint32_t);
                const size_t samples = lsp_max(nSamples, GLuint(1));
                return size_t(nWidth) * size_t(nHeight) * szof * samples;
            }

            status_t Texture::begin_draw(size_t width, size_t height, texture_format_t format)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if (format == gl::TEXTURE_UNKNOWN)
                    return STATUS_INVALID_VALUE;

                const uint32_t samples  = pContext->multisample();
                const vtbl_t *vtbl      = pContext->vtbl();
                const bool cap_changed  = (nWidth != width) || (nHeight != height) || (nSamples != samples);
                bool failed             = true;
                bool clear              = false;

                // Ensure that we have associated frame buffer
                GLuint fb_id        = nFrameBufferId;
                if (fb_id == 0)
                {
                    // Allocate framebuffer
                    fb_id = allocate_framebuffer();
                    if (fb_id == 0)
                        return STATUS_NO_MEM;

                    // Setup clear flag
                    clear           = true;
                }

                // Bind framebuffer
                vtbl->glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
                lsp_finally {
                    if (failed)
                        vtbl->glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
                };

                // Ensure that stencil buffer is set and has proper parameters
                GLuint stencil_id = nStencilBufferId;
                if ((stencil_id == 0) || (cap_changed))
                {
                    // Allocate stencil buffer if needed
                    stencil_id  = allocate_stencil();
                    if (stencil_id == 0)
                        return STATUS_NO_MEM;

                    // Initialize stencil buffer
                    vtbl->glBindRenderbuffer(GL_RENDERBUFFER, stencil_id);
                    if (samples > 0)
                        vtbl->glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_STENCIL_INDEX8, width, height);
                    else
                        vtbl->glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
                    vtbl->glBindRenderbuffer(GL_RENDERBUFFER, GL_NONE);

                    // Setup clear flag
                    clear           = true;
                }

                // Bind stencil buffer
                vtbl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_id);
                lsp_finally {
                    if (failed)
                        vtbl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
                };

                // Check that we need to re-allocate texture parameters
                GLuint texture_id   = nTextureId;
                const GLuint tex_kind   = (samples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

                if ((texture_id == 0) || (cap_changed) || (enFormat != format))
                {
                    // Allocate texture if needed
                    texture_id          = allocate_texture();
                    if (texture_id == 0)
                        return STATUS_NO_MEM;

                    const GLuint int_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                    // Bind texture and set size
                    vtbl->glBindTexture(tex_kind, texture_id);
                    if (tex_kind == GL_TEXTURE_2D_MULTISAMPLE)
                        vtbl->glTexImage2DMultisample(tex_kind, samples, int_format, width, height, GL_TRUE);
                    else
                        vtbl->glTexImage2D(tex_kind, 0, int_format, width, height, 0, int_format, GL_UNSIGNED_BYTE, NULL);

                    // Store size of texture
                    nWidth          = uint32_t(width);
                    nHeight         = uint32_t(height);
                    enFormat        = format;
                    nSamples        = (tex_kind == GL_TEXTURE_2D_MULTISAMPLE) ? GLuint(samples) : GLuint(0);

                    // Setup clear flag
                    clear           = true;
                }
                else
                {
                    // Just bind texture
                    vtbl->glBindTexture(tex_kind, texture_id);
                }
                lsp_finally {
                    if (failed)
                        vtbl->glBindTexture(tex_kind, GL_NONE);
                };

                // Attach texture to framebuffer
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                vtbl->glTexParameteri(tex_kind, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                // Attach texture to framebuffer
                vtbl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_kind, texture_id, 0);
                lsp_finally {
                    if (failed)
                        vtbl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_kind, 0, 0);
                };

                // Set the list of draw buffers
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                vtbl->glDrawBuffers(1, buffers);
                lsp_finally {
                    if (failed)
                        vtbl->glDrawBuffers(0, NULL);
                };

                // Check frame buffer status
                const GLenum status = vtbl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
                failed        = (status != GL_FRAMEBUFFER_COMPLETE);

                if (failed)
                {
                    lsp_warn("Framebuffer status: 0x%x", int(status));
                    return STATUS_UNKNOWN_ERR;
                }

                // Clear surface if if was just allocated or resized
                if (clear)
                {
                    vtbl->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    vtbl->glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                }

                return STATUS_OK;
            }

            void Texture::end_draw()
            {
                const vtbl_t *vtbl  = pContext->vtbl();
                const GLuint tex_kind   = (nSamples > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

                // Reset the list of draw buffers
                vtbl->glDrawBuffers(0, NULL);

                // Unbind resources from frame buffer
                if (nFrameBufferId != 0)
                {
                    // Unbind stencil buffer if present
                    if (nStencilBufferId != 0)
                        vtbl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

                    // Unbind texture
                    vtbl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_kind, 0, 0);
                    vtbl->glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
                }
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
