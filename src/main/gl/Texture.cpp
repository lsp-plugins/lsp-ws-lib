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

#include <private/gl/Texture.h>

#include <GL/gl.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            Texture::Texture(gl::IContext *ctx)
            {
                pContext        = safe_acquire(ctx);
                atomic_store(&nReferences, 1);
                nTextureId      = 0;
                nWidth          = 0;
                nHeight         = 0;
                enFormat        = gl::TEXTURE_UNKNOWN;
                nMulti          = 0;
            }

            Texture::~Texture()
            {
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

            status_t Texture::init_multisample(size_t width, size_t height, texture_format_t format, size_t samples)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if (format == gl::TEXTURE_UNKNOWN)
                    return STATUS_INVALID_VALUE;

                const vtbl_t *vtbl = pContext->vtbl();
                const GLuint int_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                if (nTextureId == 0)
                {
                    vtbl->glGenTextures(1, &nTextureId);
                    if (nTextureId == 0)
                        return STATUS_NO_MEM;
                }

                if (samples > 0)
                {
                    vtbl->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, nTextureId);
                    vtbl->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, int_format, width, height, GL_TRUE);
                    vtbl->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
                }
                else
                {
                    vtbl->glBindTexture(GL_TEXTURE_2D, nTextureId);
                    vtbl->glTexImage2D(GL_TEXTURE_2D, 0, int_format, width, height, 0, int_format, GL_UNSIGNED_BYTE, NULL);
                    vtbl->glBindTexture(GL_TEXTURE_2D, 0);
                }

                nWidth      = uint32_t(width);
                nHeight     = uint32_t(height);
                enFormat    = format;
                nMulti      = GLuint(samples);

                return STATUS_OK;
            }

            status_t Texture::set_image(const void *buf, size_t width, size_t height, size_t stride, texture_format_t format)
            {
                if (pContext == NULL)
                    return STATUS_BAD_STATE;
                if (format == gl::TEXTURE_UNKNOWN)
                    return STATUS_INVALID_VALUE;

                const vtbl_t *vtbl = pContext->vtbl();
                const size_t pixel_size = (format == gl::TEXTURE_ALPHA8) ? sizeof(uint8_t) : sizeof(uint32_t);
                const GLuint tex_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_BGRA;
                const GLuint int_format = (format == gl::TEXTURE_ALPHA8) ? GL_RED : GL_RGBA;

                if (nTextureId == 0)
                {
                    glGenTextures(1, &nTextureId);
                    if (nTextureId == 0)
                        return STATUS_NO_MEM;
                }

                vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / pixel_size);
                lsp_finally { vtbl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); };

                vtbl->glBindTexture(GL_TEXTURE_2D, nTextureId);
                vtbl->glTexImage2D(GL_TEXTURE_2D, 0, int_format, width, height, 0, tex_format, GL_UNSIGNED_BYTE, buf);
                vtbl->glBindTexture(GL_TEXTURE_2D, 0);

                nWidth      = uint32_t(width);
                nHeight     = uint32_t(height);
                enFormat    = format;
                nMulti      = 0;

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

                vtbl->glBindTexture(GL_TEXTURE_2D, nTextureId);
                vtbl->glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, tex_format, GL_UNSIGNED_BYTE, buf);
                vtbl->glBindTexture(GL_TEXTURE_2D, 0);

                nWidth      = lsp_max(nWidth, uint32_t(x + width));
                nHeight     = lsp_max(nHeight, uint32_t(y + height));
                nMulti      = 0;

                return STATUS_OK;
            }

            void Texture::activate(GLuint texture_id)
            {
                if (pContext == NULL)
                    return;

                const vtbl_t *vtbl = pContext->vtbl();

                const GLenum texture = (nMulti > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

                vtbl->glActiveTexture(texture_id);
                vtbl->glBindTexture(texture, nTextureId);
                vtbl->glTexParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                vtbl->glTexParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                vtbl->glTexParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                vtbl->glTexParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            void Texture::reset()
            {
                if (pContext == NULL)
                    return;

                if (nTextureId != 0)
                {
                    pContext->free_texture(nTextureId);
                    nTextureId      = 0;
                    nMulti          = 0;
                }

                safe_release(pContext);
            }

            size_t Texture::size() const
            {
                if (enFormat == gl::TEXTURE_UNKNOWN)
                    return 0;

                const size_t szof = (enFormat == gl::TEXTURE_ALPHA8) ? sizeof(uint8_t) : sizeof(uint32_t);
                const size_t samples = lsp_max(nMulti, GLuint(1));
                return size_t(nWidth) * size_t(nHeight) * szof * samples;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */


