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

#include <stddef.h>

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
//                    draw->vertices.index    = 0;

                    draw->indices.data      = NULL;
                    draw->indices.count     = 0;
                    draw->indices.capacity  = 32;
                    draw->indices.szof      = sizeof(uint8_t);

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
//                    // Store global index
//                    draw->vertices.index    = draw->vertices.count;

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
                vColors.count   = 0;

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

            #define gl_offsetof(type, field) \
                reinterpret_cast<void *>(offsetof(type, field))

            void Batch::bind_uniforms(const gl::vtbl_t *vtbl, GLuint program, const gl::uniform_t *uniform)
            {
                for ( ; (uniform != NULL) && (uniform->name != NULL); ++uniform)
                {
                    GLint location = vtbl->glGetUniformLocation(program, uniform->name);
                    if (location < 0)
                        continue;

                    switch (uniform->type)
                    {
                        case UNI_FLOAT:     vtbl->glUniform1fv(location, 1, uniform->f32); break;
                        case UNI_VEC2F:     vtbl->glUniform2fv(location, 1, uniform->f32); break;
                        case UNI_VEC3F:     vtbl->glUniform3fv(location, 1, uniform->f32); break;
                        case UNI_VEC4F:     vtbl->glUniform4fv(location, 1, uniform->f32); break;

                        case UNI_INT:       vtbl->glUniform1iv(location, 1, uniform->i32); break;
                        case UNI_VEC2I:     vtbl->glUniform2iv(location, 1, uniform->i32); break;
                        case UNI_VEC3I:     vtbl->glUniform3iv(location, 1, uniform->i32); break;
                        case UNI_VEC4I:     vtbl->glUniform4iv(location, 1, uniform->i32); break;

                        case UNI_UINT:      vtbl->glUniform1uiv(location, 1, uniform->u32); break;
                        case UNI_VEC2U:     vtbl->glUniform2uiv(location, 1, uniform->u32); break;
                        case UNI_VEC3U:     vtbl->glUniform3uiv(location, 1, uniform->u32); break;
                        case UNI_VEC4U:     vtbl->glUniform4uiv(location, 1, uniform->u32); break;

                        case UNI_MAT4F:     vtbl->glUniformMatrix4fv(location, 1, GL_FALSE, uniform->f32); break;

                        default:
                            break;
                    }
                }
            }

            status_t Batch::execute(gl::IContext *ctx, const uniform_t *uniforms)
            {
                if (pCurrent != NULL)
                    return STATUS_BAD_STATE;

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

                // Cleanup buffer
                lsp_finally { clear(); };

                // Create VBO and VAO
                const gl::vtbl_t *vtbl  = ctx->vtbl();

                GLuint VBO[2], VAO;
                vtbl->glGenBuffers(2, VBO);
                vtbl->glGenVertexArrays(1, &VAO);
                vtbl->glBindVertexArray(VAO);

                lsp_finally {
                    vtbl->glDeleteVertexArrays(1, &VAO);
                    vtbl->glDeleteBuffers(2, VBO);
                };

                // Apply batches
                size_t program_id = 0;
                size_t prev_program_id = size_t(-1);

                for (size_t i=0, n=vBatches.size(); i<n; ++i)
                {
                    draw_t *draw    = vBatches.uget(i);

                    if (draw->header.enProgram == SIMPLE)
                    {
                        // Get the program
                        status_t res = ctx->program(&program_id, gl::GEOMETRY);
                        if (res != STATUS_OK)
                            return res;

                        // Enable program
                        if (prev_program_id != program_id)
                        {
                            prev_program_id     = program_id;
                            vtbl->glUseProgram(program_id);
                            bind_uniforms(vtbl, program_id, uniforms);
                        }

                        const GLint a_vertex = vtbl->glGetAttribLocation(program_id, "a_vertex");

                        vtbl->glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
                        vtbl->glBufferData(GL_ARRAY_BUFFER, draw->vertices.count * sizeof(vertex_t), draw->vertices.v, GL_DYNAMIC_DRAW);

                        vtbl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[1]);
                        vtbl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw->indices.count * draw->indices.szof, draw->indices.data, GL_DYNAMIC_DRAW);

                        // position attribute
                        if (a_vertex >= 0)
                        {
                            vtbl->glVertexAttribPointer(a_vertex, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), gl_offsetof(vertex_t, x));
                            vtbl->glEnableVertexAttribArray(a_vertex);
                        }

//                        // color attribute
//                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//                        glEnableVertexAttribArray(1);

//                        glDrawArrays(GL_TRIANGLES, 0, draw->indices.count);
                        const GLenum index_type =
                            (draw->indices.szof > sizeof(uint16_t)) ? GL_UNSIGNED_INT :
                            (draw->indices.szof > sizeof(uint8_t)) ? GL_UNSIGNED_SHORT :
                            GL_UNSIGNED_BYTE;
                        glDrawElements(GL_TRIANGLES, draw->indices.count, index_type, NULL);

                        vtbl->glBindBuffer(GL_ARRAY_BUFFER, 0);
                        vtbl->glBindVertexArray(0);
                    }
                    else
                    {
                        status_t res = ctx->program(&program_id, gl::MULTIPOLYGON);
                        if (res != STATUS_OK)
                            return res;

                        // Enable program
                        if (prev_program_id != program_id)
                        {
                            prev_program_id     = program_id;
                            vtbl->glUseProgram(program_id);
                            bind_uniforms(vtbl, program_id, uniforms);
                        }
                    }
                }

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
                const uint32_t index    = buf.count++;
                vertex_t *v             = &buf.v[index];
                v->x                    = x;
                v->y                    = y;
                v->z                    = z;

                return index;
            }

            ssize_t Batch::alloc_indices(size_t count, size_t max_index)
            {
                // Check indices
                IF_DEBUG(
                    if (max_index > UINT32_MAX)
                        return -STATUS_OVERFLOW;
                );

                // Check indices
                ibuffer_t & buf     = pCurrent->indices;
                const size_t new_size   = buf.count + count;
                const size_t szof       =
                    (max_index > UINT16_MAX) ? sizeof(uint32_t) :
                    (max_index > UINT8_MAX) ? sizeof(uint16_t) :
                    sizeof(uint8_t);

                // Check if we need to resize the buffer
                if ((new_size > buf.capacity) || (szof > buf.szof))
                {
                    // Check if we need to widen the indices
                    const size_t new_cap    = (new_size > buf.capacity) ? buf.capacity << 1 : buf.capacity;
                    void *data              = NULL;

                    if (szof > buf.szof)
                    {
                        if ((data = malloc(new_cap * szof)) == NULL)
                            return -STATUS_NO_MEM;

                        // Perform widening
                        if (szof > sizeof(uint16_t))
                        {
                            if (buf.szof > sizeof(uint8_t))
                                convert_index(static_cast<uint32_t *>(data), buf.u16, buf.count); // Widen u16 -> u32
                            else
                                convert_index(static_cast<uint32_t *>(data), buf.u8, buf.count); // Widen u8 -> u32
                        }
                        else if (szof > sizeof(uint8_t))
                            convert_index(static_cast<uint16_t *>(data), buf.u8, buf.count); // Widen u8 -> u16
                        else
                            return -STATUS_BAD_STATE;

                        // Free previously allocated buffer
                        buf.szof            = szof;
                        free(buf.data);
                    }
                    else
                    {
                        // We can perform simple realloc operation
                        if ((data = realloc(buf.data, new_cap * buf.szof)) == NULL)
                            return -STATUS_NO_MEM;
                    }

                    // Store new pointer
                    buf.data            = data;
                    buf.capacity        = new_cap;
                }

                // Return allocated indices
                const ssize_t result    = buf.count;
                buf.count              += count;

                return result;
            }

            ssize_t Batch::triangle(size_t a, size_t b, size_t c)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                const ssize_t index     = alloc_indices(3, lsp_max(a, b, c));
                if (index < 0)
                    return index;

                // Append vertex indices
                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = uint32_t(a);
                    buf.u32[index+1]    = uint32_t(b);
                    buf.u32[index+2]    = uint32_t(c);
                }
                else if (buf.szof > sizeof(uint8_t))
                {
                    buf.u16[index]      = uint16_t(a);
                    buf.u16[index+1]    = uint16_t(b);
                    buf.u16[index+2]    = uint16_t(c);
                }
                else
                {
                    buf.u8[index]       = uint8_t(a);
                    buf.u8[index+1]     = uint8_t(b);
                    buf.u8[index+2]     = uint8_t(c);
                }

                return index;
            }

            ssize_t Batch::rectangle(size_t a, size_t b, size_t c, size_t d)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                const ssize_t index     = alloc_indices(6, lsp_max(a, b, c, d));
                if (index < 0)
                    return index;

                // Append vertex indices
                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = uint32_t(a);
                    buf.u32[index+1]    = uint32_t(b);
                    buf.u32[index+2]    = uint32_t(c);
                    buf.u32[index+3]    = uint32_t(c);
                    buf.u32[index+4]    = uint32_t(d);
                    buf.u32[index+5]    = uint32_t(a);
                }
                else if (buf.szof > sizeof(uint8_t))
                {
                    buf.u16[index]      = uint16_t(a);
                    buf.u16[index+1]    = uint16_t(b);
                    buf.u16[index+2]    = uint16_t(c);
                    buf.u16[index+3]    = uint16_t(c);
                    buf.u16[index+4]    = uint16_t(d);
                    buf.u16[index+5]    = uint16_t(a);
                }
                else
                {
                    buf.u8[index]       = uint8_t(a);
                    buf.u8[index+1]     = uint8_t(b);
                    buf.u8[index+2]     = uint8_t(c);
                    buf.u8[index+3]     = uint8_t(c);
                    buf.u8[index+4]     = uint8_t(d);
                    buf.u8[index+5]     = uint8_t(a);
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
                    const size_t new_cap    = buf.capacity << 2;
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
