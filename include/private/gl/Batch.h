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

#include <private/gl/IContext.h>
#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            enum batch_flags_t
            {
                BATCH_STENCIL_OP_MASK       = 0x03 << 0,
                BATCH_STENCIL_OP_NONE       = 0x00 << 0,
                BATCH_STENCIL_OP_OR         = 0x01 << 0,
                BATCH_STENCIL_OP_XOR        = 0x02 << 0,
                BATCH_STENCIL_OP_APPLY      = 0x03 << 0,

                BATCH_MULTISAMPLE           = 1 << 2,
                BATCH_WRITE_COLOR           = 1 << 3,
                BATCH_CLEAR_STENCIL         = 1 << 4,
                BATCH_NO_BLENDING           = 1 << 5,

                BATCH_IMPORTANT_FLAGS       = BATCH_CLEAR_STENCIL,
            };

            typedef struct LSP_HIDDEN_MODIFIER batch_header_t
            {
                gl::program_t   enProgram;  // Used program for rendering
                int32_t         nLeft;      // Origin offset left
                int32_t         nTop;       // Origin offset top
                uint32_t        nFlags;     // Flags
                gl::Texture    *pTexture;   // Related texture
            } batch_header_t;

            enum uniform_type_t
            {
                UNI_NONE,

                UNI_FLOAT,
                UNI_VEC2F,
                UNI_VEC3F,
                UNI_VEC4F,

                UNI_INT,
                UNI_VEC2I,
                UNI_VEC3I,
                UNI_VEC4I,

                UNI_UINT,
                UNI_VEC2U,
                UNI_VEC3U,
                UNI_VEC4U,

                UNI_MAT4F,
            };

            typedef struct LSP_HIDDEN_MODIFIER uniform_t
            {
                const char     *name;
                uniform_type_t  type;
                union {
                    const void    *raw;
                    const GLfloat *f32;
                    const GLint   *i32;
                    const GLuint  *u32;
                };
            } uniform_t;

            typedef struct vertex_t
            {
                float       x;      // X Coordinate
                float       y;      // Y Coordinate
                float       s;      // Texture Coordinate S
                float       t;      // Texture Coordinate T
                uint32_t    cmd;    // Draw command
            } vertex_t;

            class LSP_HIDDEN_MODIFIER Batch
            {
                private:
                    typedef struct vbuffer_t
                    {
                        vertex_t   *v;
                        uint32_t    count;
                        uint32_t    capacity;
                    } vbuffer_t;

                    typedef struct ibuffer_t
                    {
                        union {
                            uint8_t    *u8;
                            uint16_t   *u16;
                            uint32_t   *u32;
                            void       *data;
                        };

                        uint32_t    count;
                        uint32_t    capacity;
                        uint32_t    szof;
                    } ibuffer_t;

                    typedef struct cbuffer_t
                    {
                        float      *data;       // Pointer to actual data
                        uint32_t    count;      // Number of filled floats (should be always multiple of 4)
                        uint32_t    size;       // Texture size
                        uint32_t    capacity;   // Overall capacity in RGBAF32 components (4 floats per record)
                    } cbuffer_t;

                    typedef struct draw_t
                    {
                        batch_header_t  header;
                        vbuffer_t       vertices;
                        ibuffer_t       indices;
                    } draw_t;

                private:
                    cbuffer_t               vCommands;
                    lltl::parray<draw_t>    vBatches;
                    draw_t                 *pCurrent;

                private:
                    static inline bool header_mismatch(const batch_header_t & a, const batch_header_t & b);
                    static void destroy(draw_t *draw);

                    static void bind_uniforms(const gl::vtbl_t *vtbl, GLuint program, const gl::uniform_t *uniform);

                private:
                    ssize_t         alloc_indices(size_t count, uint32_t max_index);
                    ssize_t         alloc_vertices(size_t count);

                public:
                    Batch();
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
                     * Add rectangle with hint
                     * @param a index of the first vertex
                     * @param b index of the second vertex
                     * @param c index of the third vertex
                     * @param d index of the fourth vertex, should be not less than a, b and c
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t hrectangle(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

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
