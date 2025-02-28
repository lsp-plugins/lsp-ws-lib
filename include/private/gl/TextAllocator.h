/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 3 февр. 2025 г.
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

#ifndef PRIVATE_GL_TEXTALLOCATOR_H_
#define PRIVATE_GL_TEXTALLOCATOR_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/ws/types.h>

#include <private/gl/IContext.h>
#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            constexpr size_t TEXT_ATLAS_SIZE        = 512;
            constexpr float TEXT_ATLAS_SCALE        = 1.0f / TEXT_ATLAS_SIZE;

            /**
             * Class that places text fragments on a single texture
             */
            class LSP_HIDDEN_MODIFIER TextAllocator
            {
                protected:
                    typedef struct row_t
                    {
                        uint32_t        nTop;       // Top offset from beginning of the texture
                        uint32_t        nHeight;    // Height of the row
                        uint32_t        nWidth;     // Width of the row
                        gl::Texture    *pTexture;   // Related texture
                    } row_t;

                protected:
                    uatomic_t           nReferences;
                    gl::IContext       *pContext;
                    gl::Texture        *pTexture;   // Current texture
                    uint32_t            nTop;       // Row allocation position
                    lltl::darray<row_t> vRows;      // Allocation rows sorted by the height

                protected:
                    size_t              first_row_id(size_t height);
                    gl::Texture        *fill_texture(ws::rectangle_t *rect, row_t *row, const void *data, size_t width, size_t stride);

                public:
                    TextAllocator(gl::IContext *ctx);
                    TextAllocator(const TextAllocator &) = delete;
                    TextAllocator(TextAllocator &&) = delete;
                    TextAllocator & operator = (const TextAllocator &) = delete;
                    TextAllocator & operator = (TextAllocator &&) = delete;
                    ~TextAllocator();

                public:
                    uatomic_t           reference_up();
                    uatomic_t           reference_down();

                public:
                    /**
                     * Allocate fragment of a 2D texture and fill it with provided data
                     * @param rect pointer to store the position of the allocated region within the texture
                     * @param data 8-bit alpha-channel data
                     * @param width width of the alpha-channel data
                     * @param height height of the alpha-channel data
                     * @param stride stride in bytes between rows
                     * @return pointer to the texture on success or NULL on error.
                     */
                    gl::Texture        *allocate(ws::rectangle_t *rect, const void *data, size_t width, size_t height, size_t stride);

                    /**
                     * Get current texture atlas
                     * @return current texture atlas
                     */
                    gl::Texture        *current();

                    /**
                     * Cleanup allocator state
                     */
                    void                clear();
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_TEXTALLOCATOR_H_ */
