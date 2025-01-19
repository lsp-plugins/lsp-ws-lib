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
            enum batch_program_t
            {
                SIMPLE,
                STENCIL
            };

            typedef struct batch_header_t
            {
                batch_program_t enProgram;
                bool            bMultisampling;
            } batch_header_t;


            class Batch
            {
                private:
                    typedef struct vertex_t
                    {
                        float       x;
                        float       y;
                        float       z;
                    } vertex_t;

                    typedef struct color_t
                    {
                        float       r;
                        float       g;
                        float       b;
                        float       a;
                    } color_t;

                    typedef struct vbuffer_t
                    {
                        vertex_t   *v;
                        size_t      count;
                        size_t      capacity;
                    } vbuffer_t;

                    typedef struct ibuffer_t
                    {
                        union {
                            uint8_t    *u8;
                            uint16_t   *u16;
                            uint32_t   *u32;
                            void       *data;
                        };

                        size_t      count;
                        size_t      capacity;
                        uint32_t    index;
                        uint32_t    limit;
                    } ibuffer_t;

                    typedef struct cbuffer_t
                    {
                        color_t    *c;
                        size_t      count;
                        size_t      capacity;
                    } cbuffer_t;

                    typedef struct draw_t
                    {
                        batch_header_t  header;
                        vbuffer_t       vertices;
                        ibuffer_t       indices;
                    } draw_t;

                private:
                    cbuffer_t               vColors;
                    lltl::parray<draw_t>    vBatches;
                    draw_t                 *pCurrent;

                private:
                    static inline bool header_mismatch(const batch_header_t & a, const batch_header_t & b);
                    static void destroy(draw_t *draw);

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
                     * @return status of operation
                     */
                    status_t execute(gl::IContext *ctx);

                    /**
                     * Clear batch
                     */
                    void clear();

                public:
                    /**
                     * Add vertex
                     * @param x vertex X coordinate
                     * @param y vertex Y coordinate
                     * @param z vertex Z coordinate
                     * @return absolute index of vertex in vertex buffer or negative error code
                     */
                    ssize_t vertex(float x, float y, float z);

                    /**
                     * Add triangle
                     * @param a relative index of the first vertex
                     * @param b relative index of the second vertex
                     * @param c relative index of the third vertex
                     * @return absolute index of record in index buffer or negative error code
                     */
                    ssize_t triangle(size_t a, size_t b, size_t c);

                    /**
                     * Add color
                     * @param r red component
                     * @param g green component
                     * @param b blue component
                     * @param a alpha component
                     * @return absolute index of color in color buffer or negative error code
                     */
                    ssize_t color(float r, float g, float b, float a);
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_GL_BATCH_H_ */
