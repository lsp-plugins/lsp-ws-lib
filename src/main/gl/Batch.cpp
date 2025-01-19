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

#include <private/gl/Batch.h>

#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            inline bool Batch::header_mismatch(const batch_header_t & a, const batch_header_t & b)
            {
                return (a.enProgram != b.enProgram) || (a.bMultisampling != b.bMultisampling);
            }

            template<class D, class S>
            inline void convert_index(D *dst, const S *src, size_t count)
            {
                for (size_t i=0; i<count; ++i)
                    dst[i]      = src[i];
            }

            void Batch::destroy(draw_t *draw)
            {
                if (draw == NULL)
                    return;

                if (draw->vertices.v != NULL)
                {
                    free(draw->vertices.v);
                    draw->vertices.v        = NULL;
                }
                if (draw->indices.data != NULL)
                {
                    free(draw->indices.data);
                    draw->indices.data      = NULL;
                }

                free(draw);
            }

            Batch::Batch()
            {
                vColors.c           = NULL;
                vColors.count       = 0;
                vColors.capacity    = 0;
                pCurrent            = NULL;
            }

            Batch::~Batch()
            {
                clear();
                if (vColors.c != NULL)
                {
                    free(vColors.c);
                    vColors.c           = NULL;
                }
            }

            status_t Batch::init()
            {
                constexpr size_t start_capacity = 16;
                vColors.c           = static_cast<color_t *>(malloc(sizeof(color_t) * start_capacity * start_capacity));
                if (vColors.c == NULL)
                    return STATUS_NO_MEM;

                vColors.count       = 0;
                vColors.capacity    = start_capacity * start_capacity;

                bzero(vColors.c, vColors.capacity * sizeof(color_t));

                return STATUS_OK;
            }

            status_t Batch::begin(const batch_header_t & header)
            {
                draw_t *draw = vBatches.last();
                if ((draw == NULL) || (header_mismatch(draw->header, header)))
                {
                    // Create new draw batch
                    draw                    = static_cast<draw_t *>(malloc(sizeof(draw_t)));
                    if (draw == NULL)
                        return STATUS_NO_MEM;
                    draw->header            = header;
                    draw->vertices.v        = NULL;
                    draw->vertices.count    = 0;
                    draw->vertices.capacity = 32;

                    draw->indices.data      = NULL;
                    draw->indices.count     = 0;
                    draw->indices.capacity  = 32;
                    draw->indices.index     = 0;
                    draw->indices.limit     = UINT8_MAX;

                    lsp_finally { destroy(draw); };

                    // Initialize new draw batch
                    draw->vertices.v        = static_cast<vertex_t *>(malloc(sizeof(vertex_t) * draw->vertices.capacity));
                    if (draw->vertices.v == NULL)
                        return STATUS_NO_MEM;
                    draw->indices.data      = malloc(sizeof(uint8_t) * draw->indices.capacity);
                    if (draw->indices.data == NULL)
                        return STATUS_NO_MEM;

                    // Add new batch to list
                    if (!vBatches.add(draw))
                        return STATUS_NO_MEM;

                    // Set current batch
                    pCurrent                = release_ptr(draw);
                }
                else
                {
                    // Store global index
                    draw->indices.index     = draw->indices.count;

                    // Set current batch
                    pCurrent                = draw;
                }

                return STATUS_OK;
            }

            void Batch::clear()
            {
                // Destroy all batches except current
                for (size_t i=0, n=vBatches.size(); i < n; ++i)
                {
                    draw_t *draw = vBatches.uget(i);
                    if (draw != pCurrent)
                        destroy(draw);
                }
                vBatches.clear();

                // Try to add current batch to the list of batches
                if (pCurrent != NULL)
                {
                    if (!vBatches.add(pCurrent))
                    {
                        destroy(pCurrent);
                        pCurrent        = NULL;
                    }
                }
            }

            status_t Batch::end()
            {
                if (pCurrent == NULL)
                    return STATUS_BAD_STATE;

                pCurrent    = NULL;
                return STATUS_OK;
            }

            status_t Batch::execute(gl::IContext *ctx)
            {
                IF_TRACE(
                    lsp_trace("Batch: draws=%d,  colors=%d", int(vBatches.size()), int(vColors.count));

                    for (size_t i=0, n=vBatches.size(); i<n; ++i)
                    {
                        draw_t *draw    = vBatches.uget(i);
                        const char *type = "unknown";
                        if (draw->header.enProgram == SIMPLE)
                            type    = "simple";
                        else if (draw->header.enProgram == STENCIL)
                            type    = "stencil";

                        lsp_trace("  draw #%3d: type=%s, multisampling=%s, vertices=%d, indices=%d",
                            int(i), type, (draw->header.bMultisampling) ? " true" : "false",
                            int(draw->vertices.count), int (draw->indices.count));

                    }
                );

                // Cleanup color mapping
                clear();
                vColors.count       = 0;

                // TODO
                return STATUS_OK;
            }

            ssize_t Batch::vertex(float x, float y, float z)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                // Check if we need to resize the buffer
                vbuffer_t & buf = pCurrent->vertices;
                if (buf.count >= buf.capacity)
                {
                    const size_t new_cap    = buf.capacity << 1;
                    vertex_t *ptr           = static_cast<vertex_t *>(realloc(buf.v, sizeof(vertex_t) * new_cap));
                    if (ptr == NULL)
                        return -STATUS_NO_MEM;
                    buf.v                   = ptr;
                    buf.capacity            = new_cap;
                }

                // Append vertex
                const size_t index  = buf.count++;
                vertex_t *v         = &buf.v[index];
                v->x                = x;
                v->y                = y;
                v->z                = z;

                return index;
            }

            ssize_t Batch::triangle(size_t a, size_t b, size_t c)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                // Check indices
                ibuffer_t & buf     = pCurrent->indices;
                const size_t off    = buf.index;
                a                  += off;
                b                  += off;
                c                  += off;

                const size_t max_index  = lsp_max(a, b, c);
                IF_DEBUG(
                    if (max_index > UINT32_MAX)
                        return -STATUS_OVERFLOW;
                );

                const size_t new_size   = buf.count + 3;

                // Check if we need to resize the buffer
                if ((new_size >= buf.capacity) || (max_index > buf.limit))
                {
                    // Check if we need to widen the indices
                    const size_t new_cap    = (new_size > buf.capacity) ? buf.capacity << 1 : buf.capacity;
                    const size_t limit      = lsp_max(max_index, buf.limit);
                    const size_t szof       =
                        (limit > UINT16_MAX) ? sizeof(uint32_t) :
                        (limit > UINT8_MAX) ? sizeof(uint16_t) :
                        sizeof(uint8_t);

                    void *data              = NULL;
                    if (max_index > buf.limit)
                    {
                        if ((data = malloc(new_cap * szof)) == NULL)
                            return -STATUS_NO_MEM;

                        // Perform widening
                        if (max_index > UINT16_MAX)
                        {
                            if (buf.limit > UINT8_MAX)
                            {
                                // Widen u16 -> u32
                                convert_index(static_cast<uint32_t *>(data), buf.u16, buf.count);
                                buf.limit           = UINT32_MAX;
                            }
                            else
                            {
                                // Widen u8 -> u32
                                convert_index(static_cast<uint32_t *>(data), buf.u8, buf.count);
                                buf.limit           = UINT32_MAX;
                            }
                        }
                        else if (max_index > UINT8_MAX)
                        {
                            // Widen u8 -> u16
                            convert_index(static_cast<uint16_t *>(data), buf.u8, buf.count);
                            buf.limit           = UINT16_MAX;
                        }
                        else
                            return -STATUS_BAD_STATE;

                        // Free previously allocated buffer
                        free(buf.data);
                    }
                    else
                    {
                        // We can perform simple realloc operation
                        if ((data = realloc(buf.data, new_cap * szof)) == NULL)
                            return -STATUS_NO_MEM;
                    }

                    // Store new pointer
                    buf.data            = data;
                    buf.capacity        = new_cap;
                }

                // Append vertex indices
                const size_t index  = buf.count;
                buf.count          += 3;

                if (buf.limit > UINT16_MAX)
                {
                    buf.u32[index]  = uint32_t(a);
                    buf.u32[index+1]= uint32_t(b);
                    buf.u32[index+2]= uint32_t(c);
                }
                else if (buf.limit > UINT8_MAX)
                {
                    buf.u16[index]  = uint16_t(a);
                    buf.u16[index+1]= uint16_t(b);
                    buf.u16[index+2]= uint16_t(c);
                }
                else
                {
                    buf.u8[index]   = uint8_t(a);
                    buf.u8[index+1] = uint8_t(b);
                    buf.u8[index+2] = uint8_t(c);
                }

                return index;
            }

            ssize_t Batch::color(float r, float g, float b, float a)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                // Check if we need to resize the buffer
                cbuffer_t & buf = vColors;
                if (buf.count >= buf.capacity)
                {
                    const size_t new_cap    = buf.capacity << 1;
                    color_t *ptr            = static_cast<color_t *>(realloc(buf.c, sizeof(color_t) * new_cap));
                    if (ptr == NULL)
                        return -STATUS_NO_MEM;
                    buf.c                   = ptr;
                    buf.capacity            = new_cap;
                }

                // Append vertex
                const size_t index  = buf.count++;
                color_t *v          = &buf.c[index];
                v->r                = r;
                v->g                = g;
                v->b                = b;
                v->a                = a;

                return index;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */
