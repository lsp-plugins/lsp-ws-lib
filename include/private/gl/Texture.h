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

#ifndef PRIVATE_GL_TEXTURE_H_
#define PRIVATE_GL_TEXTURE_H_

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/IContext.h>

#include <GL/gl.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            enum texture_format_t
            {
                TEXTURE_UNKNOWN,    // No texture format specified
                TEXTURE_RGBA32,     // 32-bit color with 8 bits per red, gree, blue and alpha components
                TEXTURE_ALPHA8      // 8-bit alpha component
            };

            class LSP_HIDDEN_MODIFIER Texture
            {
                private:
                    gl::IContext       *pContext;
                    uatomic_t           nReferences;
                    GLuint              nTextureId;
                    uint32_t            nWidth;
                    uint32_t            nHeight;
                    texture_format_t    enFormat;

                public:
                    Texture(IContext *ctx);
                    Texture(const Texture &) = delete;
                    Texture(Texture &&) = delete;
                    ~Texture();

                    Texture & operator = (const Texture &) = delete;
                    Texture & operator = (Texture &&) = delete;

                public:
                    uatomic_t           reference_up();
                    uatomic_t           reference_down();

                public:
                    status_t            set_image(const void *buf, size_t width, size_t height, size_t stride, texture_format_t format);
                    status_t            set_subimage(const void *buf, size_t x, size_t y, size_t width, size_t height, size_t stride);
                    void                activate(GLuint texture_id);
                    void                reset();

                public:
                    inline bool valid() const { return enFormat != TEXTURE_UNKNOWN; }
                    inline texture_format_t format() const { return enFormat; }
                    inline uint32_t width() const { return nWidth; }
                    inline uint32_t height() const { return nHeight; }
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */



#endif /* PRIVATE_GL_TEXTURE_H_ */
