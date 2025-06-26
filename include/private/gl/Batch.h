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

#ifndef PRIVATE_GL_BATCH_H_
#define PRIVATE_GL_BATCH_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/Allocator.h>
#include <private/gl/IContext.h>
#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class LSP_HIDDEN_MODIFIER Batch
            {
                private:
                    batch_cbuffer_t             vCommands;
                    lltl::parray<batch_draw_t>  vBatches;
                    batch_draw_t               *pCurrent;
                    Allocator                  *pAllocator;

                private:
                    static inline bool header_mismatch(const batch_header_t & a, const batch_header_t & b);

                    static void bind_uniforms(const gl::vtbl_t *vtbl, GLuint program, const gl::uniform_t *uniform);

                private:
                    ssize_t         alloc_indices(size_t count, uint32_t max_index);
                    ssize_t         alloc_vertices(size_t count);

                public:
                    Batch(Allocator * alloc);
                    Batch(const Batch &) = delete;
                    Batch(Batch &&) = delete;
                    ~Batch();
                    Batch & operator = (const Batch &) = delete;
                    Batch & operator = (Batch &&) = delete;

                public:
                    /**
                     * Initialize batch
                     * @return status of operation
                     */
                    status_t init();

                    /**
                     * Start batch
                     * @param header batch header
                     * @return status of operation
                     */
                    status_t begin(const batch_header_t & header);

                    /**
                     * End batch
                     * @param header batch header
                     * @return status of operation
                     */
                    status_t end();

                    /**
                     * Execute batch on a context
                     * @param ctx context
                     * @param uniforms uniforms for rendering
                     * @return status of operation
                     */
                    status_t execute(gl::IContext *ctx, const uniform_t *uniforms);

                    /**
                     * Clear batch
                     */
                    void clear();

                public:
                    /**
                     * Add vertex
                     * @param cmd command for a vertex
                     * @param x vertex X coordinate
                     * @param y vertex Y coordinate
                     * @return relative to the beginning of batch index of vertex in vertex buffer or negative error code
                     */
                    ssize_t vertex(uint32_t cmd, float x, float y);

                    /**
                     * Add vertex with texture coordinates
                     * @param cmd command for a vertex
                     * @param x vertex X coordinate
                     * @param y vertex Y coordinate
                     * @param s texture coordinate S
                     * @param t texture coordinate T
                     * @return relative to the beginning of batch index of vertex in vertex buffer or negative error code
                     */
                    ssize_t textured_vertex(uint32_t cmd, float x, float y, float s, float t);

                    /**
                     * Identifier of the next vertex that will be allocated on addition call
                     * @return identifier of the next allocated vertex
                     */
                    inline uint32_t next_vertex_index() const { return pCurrent->vertices.count; }

                    /**
                     * Add new set of vertices
                     * @param count number of vertices to add
                     * @return pointer to first added vertex or NULL
                     */
                    vertex_t *add_vertices(size_t count);

                    /**
                     * Release set of vertices at the tail of the buffer
                     * @param count number of vertices to release
                     */
                    void release_vertices(size_t count);

                    /**
                     * Add indices. The index element size can be 1, 2 or 4 bytes size, so you need
                     * to check the size by issuing index_format() function
                     * @param count number of indices to add
                     * @param max_value maximum value stored to index
                     */
                    void *add_indices(size_t count, size_t max_value);

                    /**
                     * Release set of indices at the tail of the buffer
                     * @param count number of indices to release
                     */
                    void release_indices(size_t count);

                    /**
                     * Get format of the index item
                     * @return format of the index item
                     */
                    index_format_t index_format() const;

                    /**
                     * Add triangle
                     * @param a index of the first vertex
                     * @param b index of the second vertex
                     * @param c index of the third vertex
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t triangle(uint32_t a, uint32_t b, uint32_t c);

                    /**
                     * Add rectangle
                     * @param a index of the first vertex
                     * @param b index of the second vertex
                     * @param c index of the third vertex
                     * @param d index of the fourth vertex
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t rectangle(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

                    /**
                     * Add triangle with hint (maximum index provided in last index)
                     * @param a index of the first vertex
                     * @param b index of the second vertex
                     * @param c index of the third vertex, should be not less than a and b
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t htriangle(uint32_t a, uint32_t b, uint32_t c);

                    /**
                     * Add indices that generate triangle fan chain
                     * @param v0i index of the first vertex
                     * @param count number of triangles
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t htriangle_fan(uint32_t v0i, uint32_t count);

                    /**
                     * Add rectangle with hint
                     * @param a index of the first vertex
                     * @param b index of the second vertex
                     * @param c index of the third vertex
                     * @param d index of the fourth vertex, should be not less than a, b and c
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t hrectangle(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

                    /**
                     * Add indices that generate rectangle fan chain
                     * @param v0i index of the first vertex
                     * @param count number of rectangles
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t hrectangle_fan(uint32_t v0i, uint32_t count);

                    /**
                     * Add command
                     * @param count length of command in 32-bit floats
                     * @param buf pointer to store the pointer to the beginning of the buffer
                     * @return index of the batch in the buffer or negative error code
                     */
                    ssize_t command(float **data, size_t count);
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_BATCH_H_ */
