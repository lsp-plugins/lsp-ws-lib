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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/IContext.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            enum texture_format_t
            {
                TEXTURE_UNKNOWN     = -1,    // No texture format specified
                TEXTURE_RGBA32      = 0,     // 8-bit alpha component
                TEXTURE_ALPHA8      = 1,     // 8-bit alpha component
                TEXTURE_PRGBA32     = 2,     // 32-bit color with 8 bits per red, gree, blue and alpha components, alpha is premultiplied
            };

            class LSP_HIDDEN_MODIFIER Texture
            {
                private:
                    gl::IContext       *pContext;
                    uatomic_t           nReferences;
                    GLuint              nTextureId;
                    GLuint              nFrameBufferId;
                    GLuint              nStencilBufferId;
                    GLuint              nProcessorId;
                    uint32_t            nWidth;
                    uint32_t            nHeight;
                    texture_format_t    enFormat;
                    GLuint              nSamples;

                protected:
                    inline GLuint       allocate_texture();
                    inline GLuint       allocate_framebuffer();
                    inline GLuint       allocate_stencil();
                    inline void         deallocate_buffers();

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
                    void                activate(GLuint processor_id);
                    status_t            resize(size_t width, size_t height);
                    void                deactivate();
                    void                reset();

                    status_t            begin_draw(size_t width, size_t height, texture_format_t format);
                    void                end_draw();

                public:
                    /**
                     * Check validity of texture
                     * @return validity of texture
                     */
                    inline bool valid() const { return enFormat != TEXTURE_UNKNOWN; }

                    /**
                     * Get texture format
                     * @return texture format
                     */
                    inline texture_format_t format() const { return enFormat; }

                    /**
                     * Get texture width
                     * @return texture width
                     */
                    inline uint32_t width() const { return nWidth; }

                    /**
                     * Get texture height
                     * @return texture height
                     */
                    inline uint32_t height() const { return nHeight; }

                    /**
                     * Get texture size
                     * @return the texture size in memory
                     */
                    size_t size() const;

                    /**
                     * Get texture identifier
                     * @return texture identifier
                     */
                    inline GLuint id() const        { return nTextureId; }

                    /**
                     * Get multisampling flag
                     * @return multisampling flag
                     */
                    inline uint32_t multisampling() const   { return nSamples; }
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_TEXTURE_H_ */
