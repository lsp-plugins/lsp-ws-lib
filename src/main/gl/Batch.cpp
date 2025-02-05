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

#ifdef LSP_PLUGINS_USE_OPENGL

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
                return (a.enProgram != b.enProgram) ||
                       (a.nFlags != b.nFlags) ||
                       (a.pTexture != b.pTexture);
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

                safe_release(draw->header.pTexture);

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
                vCommands.data      = NULL;
                vCommands.count     = 0;
                vCommands.capacity  = 0;
                pCurrent            = NULL;
            }

            Batch::~Batch()
            {
                clear();
                if (vCommands.data != NULL)
                {
                    free(vCommands.data);
                    vCommands.data      = NULL;
                }
            }

            status_t Batch::init()
            {
                constexpr size_t default_size = 32;

                vCommands.count     = 0;
                vCommands.size      = default_size;
                vCommands.capacity  = default_size * default_size;
                vCommands.data      = static_cast<float *>(malloc(vCommands.capacity * sizeof(float) * 4));
                if (vCommands.data == NULL)
                    return STATUS_NO_MEM;

                bzero(vCommands.data, vCommands.capacity * sizeof(float) * 4);

                return STATUS_OK;
            }

            status_t Batch::begin(const batch_header_t & header)
            {
                draw_t *draw            = vBatches.last();
                if ((draw == NULL) || (header_mismatch(draw->header, header)))
                {
                    // Create new draw batch
                    draw                    = static_cast<draw_t *>(malloc(sizeof(draw_t)));
                    if (draw == NULL)
                        return STATUS_NO_MEM;
                    draw->header            = header;
                    draw->vertices.v        = NULL;
                    draw->vertices.count    = 0;
                    draw->vertices.capacity = 0x40;

                    safe_acquire(draw->header.pTexture);

                    draw->indices.data      = NULL;
                    draw->indices.count     = 0;
                    draw->indices.capacity  = 0x100;
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
                    // Set current batch
                    pCurrent                = draw;

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
                vCommands.count     = 0;

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

                // Remove batch if it is empty
                if ((pCurrent->vertices.count == 0) || (pCurrent->indices.count == 0))
                {
                    if (!(pCurrent->header.nFlags & BATCH_IMPORTANT_FLAGS))
                    {
                        vBatches.pop();
                        destroy(pCurrent);
                    }
                }

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

//                lsp_trace("Batch: draws=%d,  commands=%d (%d bytes)",
//                    int(vBatches.size()),
//                    int(vCommands.count),
//                    int(vCommands.count * sizeof(float)));

                // Cleanup buffer
                lsp_finally { clear(); };

                // Create VBO and VAO
                const gl::vtbl_t *vtbl  = ctx->vtbl();

                GLuint VBO[2];
                GLuint VAO;
                vtbl->glGenBuffers(2, VBO);
                vtbl->glGenVertexArrays(1, &VAO);
                vtbl->glBindVertexArray(VAO);

                lsp_finally {
                    // Reset state
                    vtbl->glBindVertexArray(0);
                    vtbl->glDeleteVertexArrays(1, &VAO);
                    vtbl->glDeleteBuffers(2, VBO);
                    vtbl->glUseProgram(0);
                };

                // Apply batches
                size_t program_id = 0;
                size_t prev_program_id = size_t(-1);

//                IF_TRACE(
//                    size_t vertices = 0;
//                    size_t vertex_bytes = 0;
//                    size_t indices = 0;
//                    size_t index_bytes = 0;
//                    size_t textures = 0;
//                    size_t texture_bytes = 0;
//                );

                status_t res = ctx->load_command_buffer(vCommands.data, vCommands.size);
                if (res != STATUS_OK)
                    return res;

                vtbl->glDisable(GL_DEPTH_TEST);

                for (size_t i=0, n=vBatches.size(); i<n; ++i)
                {
                    draw_t *draw        = vBatches.uget(i);
                    const size_t flags  = draw->header.nFlags;

                    // Get the program
                    status_t res = ctx->program(&program_id, draw->header.enProgram);
                    if (res != STATUS_OK)
                        return res;

                    // Enable program
                    if (prev_program_id != program_id)
                    {
                        prev_program_id     = program_id;
                        vtbl->glUseProgram(program_id);
                        bind_uniforms(vtbl, program_id, uniforms);
                    }

                    // Command buffer
                    const GLint u_commands = vtbl->glGetUniformLocation(program_id, "u_commands");
                    if (u_commands > 0)
                    {
                        vtbl->glUniform1i(u_commands, 0);
                        ctx->bind_command_buffer(GL_TEXTURE0);
                    }
                    lsp_finally {
                        if (u_commands > 0)
                            ctx->unbind_command_buffer();
                    };

                    // Optional masking texture
                    const GLint u_texture = vtbl->glGetUniformLocation(program_id, "u_texture");
                    gl::Texture *mask_texture = NULL;
                    if (u_texture > 0)
                    {
                        vtbl->glUniform1i(u_texture, 1);

                        mask_texture = draw->header.pTexture;
                        if ((mask_texture != NULL) && (mask_texture->valid()))
                            mask_texture->bind(GL_TEXTURE1);
                        else
                            ctx->bind_empty_texture(GL_TEXTURE1, 0);
                    }
                    lsp_finally {
                        if (u_texture > 0)
                        {
                            if ((mask_texture != NULL) && (mask_texture->valid()))
                                mask_texture->unbind();
                            else
                                ctx->unbind_empty_texture(GL_TEXTURE1, 0);
                        }
                    };

                    // Optinal multisampled masking texture
                    const GLint u_ms_texture = vtbl->glGetUniformLocation(program_id, "u_ms_texture");
                    gl::Texture *ms_mask_texture = NULL;
                    if (u_ms_texture > 0)
                    {
                        vtbl->glUniform1i(u_ms_texture, 2);

                        ms_mask_texture = draw->header.pTexture;
                        if ((ms_mask_texture != NULL) && (ms_mask_texture->valid()))
                            ms_mask_texture->bind(GL_TEXTURE2);
                        else
                            ctx->bind_empty_texture(GL_TEXTURE2, ctx->multisample());
                    }
                    lsp_finally {
                        if (u_ms_texture > 0)
                        {
                            if ((ms_mask_texture != NULL) && (ms_mask_texture->valid()))
                                ms_mask_texture->unbind();
                            else
                                ctx->unbind_empty_texture(GL_TEXTURE2, ctx->multisample());
                        }
                    };

//                    IF_TRACE(
//                        {
//                            const char *type = "unknown";
//                            if (draw->header.enProgram == GEOMETRY)
//                                type    = "geometry";
//                            else if (draw->header.enProgram == STENCIL)
//                                type    = "stencil";
//
//                            lsp_trace("  draw #%3d: type=%s, flags=0x%08x, texture=%p (%d bytes), vertices=%d (%d bytes), indices=%d (%d bytes)",
//                                int(i), type, int(draw->header.nFlags),
//                                draw->header.pTexture,
//                                (draw->header.pTexture != NULL) ? draw->header.pTexture->size() : 0,
//                                int(draw->vertices.count), int(draw->vertices.count * sizeof(vertex_t)),
//                                int(draw->indices.count), int(draw->indices.count * draw->indices.szof));
//
//                            vertices       += draw->vertices.count;
//                            vertex_bytes   += draw->vertices.count * sizeof(vertex_t);
//                            indices        += draw->indices.count;
//                            index_bytes    += draw->indices.count * draw->indices.szof;
//                            textures       += (draw->header.pTexture != NULL) ? 1 : 0;
//                            texture_bytes  += (draw->header.pTexture != NULL) ? draw->header.pTexture->size() : 0;
//                        }
//                    );

                    // Configure stencil buffer
                    if (flags & BATCH_CLEAR_STENCIL)
                    {
                        vtbl->glStencilMask(0x01);
                        vtbl->glClear(GL_STENCIL_BUFFER_BIT);
                    }

                    // Check batch size
                    if (draw->vertices.count <= 0)
                        continue;

                    // Control multisampling
                    if (flags & BATCH_MULTISAMPLE)
                        vtbl->glEnable(GL_MULTISAMPLE);
                    else
                        vtbl->glDisable(GL_MULTISAMPLE);

                    // Blending function
                    if (flags & BATCH_NO_BLENDING)
                        vtbl->glBlendFunc(GL_ONE, GL_ZERO);
                    else
                        vtbl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    vtbl->glEnable(GL_BLEND);

                    // Configure color buffer
                    const GLboolean color_mask  = (flags & BATCH_WRITE_COLOR) ? GL_TRUE : GL_FALSE;
                    vtbl->glColorMask(color_mask, color_mask, color_mask, color_mask);

                    switch (flags & BATCH_STENCIL_OP_MASK)
                    {
                        case BATCH_STENCIL_OP_OR:
                            vtbl->glEnable(GL_STENCIL_TEST);
                            vtbl->glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                            vtbl->glStencilFunc(GL_ALWAYS, 0x01, 0x01);
                            vtbl->glStencilMask(0x01);
                            break;

                        case BATCH_STENCIL_OP_XOR:
                            vtbl->glEnable(GL_STENCIL_TEST);
                            vtbl->glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
                            vtbl->glStencilFunc(GL_ALWAYS, 0x01, 0x01);
                            vtbl->glStencilMask(0x01);
                            break;

                        case BATCH_STENCIL_OP_APPLY:
                            vtbl->glEnable(GL_STENCIL_TEST);
                            vtbl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                            vtbl->glStencilFunc(GL_EQUAL, 0x01, 0x01);
                            vtbl->glStencilMask(0x00);
                            break;

                        case BATCH_STENCIL_OP_NONE:
                        default:
                            vtbl->glDisable(GL_STENCIL_TEST);
                            vtbl->glStencilMask(0x00);
                            break;
                    }

                    // Vertex buffer
                    vtbl->glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
                    vtbl->glBufferData(GL_ARRAY_BUFFER, draw->vertices.count * sizeof(vertex_t), draw->vertices.v, GL_STATIC_DRAW);
                    lsp_finally { vtbl->glBindBuffer(GL_ARRAY_BUFFER, 0); };

                    // Element array buffer
                    vtbl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[1]);
                    vtbl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw->indices.count * draw->indices.szof, draw->indices.data, GL_STATIC_DRAW);
                    lsp_finally { vtbl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); };

                    // Bind vertex attributes
                    const GLint a_vertex = vtbl->glGetAttribLocation(program_id, "a_vertex");
                    const GLint a_texcoord = vtbl->glGetAttribLocation(program_id, "a_texcoord");
                    const GLint a_command = vtbl->glGetAttribLocation(program_id, "a_command");

                    // position attribute
                    if (a_vertex >= 0)
                    {
                        vtbl->glVertexAttribPointer(a_vertex, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), gl_offsetof(vertex_t, x));
                        vtbl->glEnableVertexAttribArray(a_vertex);
                    }
                    // texture coordinates
                    if (a_texcoord >= 0)
                    {
                        vtbl->glVertexAttribPointer(a_texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), gl_offsetof(vertex_t, s));
                        vtbl->glEnableVertexAttribArray(a_texcoord);
                    }
                    // draw command
                    if (a_command >= 0)
                    {
                        vtbl->glVertexAttribIPointer(a_command, 1, GL_UNSIGNED_INT, sizeof(vertex_t), gl_offsetof(vertex_t, cmd));
                        vtbl->glEnableVertexAttribArray(a_command);
                    }

                    const GLenum index_type =
                        (draw->indices.szof > sizeof(uint16_t)) ? GL_UNSIGNED_INT :
                        (draw->indices.szof > sizeof(uint8_t)) ? GL_UNSIGNED_SHORT :
                        GL_UNSIGNED_BYTE;

                    // Draw content
                    vtbl->glDrawElements(GL_TRIANGLES, draw->indices.count, index_type, NULL);
                }

//                IF_TRACE(
//                    lsp_trace("  TOTAL: textures=%d (%d bytes), vertices=%d (%d bytes), indices=%d (%d bytes)",
//                        int(textures), int(texture_bytes),
//                        int(vertices), int(vertex_bytes),
//                        int(indices), int(index_bytes));
//                );

                return STATUS_OK;
            }

            ssize_t Batch::alloc_vertices(size_t count)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                // Check if we need to resize the buffer
                vbuffer_t & buf = pCurrent->vertices;
                const size_t new_count = buf.count + count;
                if (new_count > buf.capacity)
                {
                    // Estimate new capacity
                    size_t new_cap          = buf.capacity << 1;
                    while (new_cap < new_count)
                        new_cap               <<= 1;

                    vertex_t *ptr           = static_cast<vertex_t *>(realloc(buf.v, sizeof(vertex_t) * new_cap));
                    if (ptr == NULL)
                        return -STATUS_NO_MEM;
                    buf.v                   = ptr;
                    buf.capacity            = new_cap;
                }

                // Allocate vertices
                const ssize_t index = buf.count;
                buf.count          += count;
                return index;
            }

            ssize_t Batch::vertex(uint32_t cmd, float x, float y)
            {
                const ssize_t index     = alloc_vertices(1);
                if (index < 0)
                    return index;

                vbuffer_t & buf         = pCurrent->vertices;
                vertex_t *v             = &buf.v[index];
                v->x                    = x;
                v->y                    = y;
                v->s                    = 0.0f;
                v->t                    = 0.0f;
                v->cmd                  = cmd;

                return index;
            }

            ssize_t Batch::textured_vertex(uint32_t cmd, float x, float y, float s, float t)
            {
                const ssize_t index     = alloc_vertices(1);
                if (index < 0)
                    return index;

                vbuffer_t & buf         = pCurrent->vertices;
                vertex_t *v             = &buf.v[index];
                v->x                    = x;
                v->y                    = y;
                v->s                    = s;
                v->t                    = t;
                v->cmd                  = cmd;

                return index;
            }

            vertex_t *Batch::add_vertices(size_t count)
            {
                const ssize_t index     = alloc_vertices(count);
                return (index >= 0) ? &pCurrent->vertices.v[index] : NULL;
            }

            ssize_t Batch::alloc_indices(size_t count, uint32_t max_index)
            {
                // Check indices
                ibuffer_t & buf         = pCurrent->indices;
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

            ssize_t Batch::triangle(uint32_t a, uint32_t b, uint32_t c)
            {
                const ssize_t index     = alloc_indices(3, lsp_max(a, b, c));
                if (index < 0)
                    return index;

                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = a;
                    buf.u32[index+1]    = b;
                    buf.u32[index+2]    = c;
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

            ssize_t Batch::htriangle(uint32_t a, uint32_t b, uint32_t c)
            {
                const ssize_t index     = alloc_indices(3, c);
                if (index < 0)
                    return index;

                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = a;
                    buf.u32[index+1]    = b;
                    buf.u32[index+2]    = c;
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

            ssize_t Batch::rectangle(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
            {
                const ssize_t index     = alloc_indices(6, lsp_max(a, b, c, d));
                if (index < 0)
                    return index;

                // Append vertex indices
                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = a;
                    buf.u32[index+1]    = b;
                    buf.u32[index+2]    = c;
                    buf.u32[index+3]    = a;
                    buf.u32[index+4]    = c;
                    buf.u32[index+5]    = d;
                }
                else if (buf.szof > sizeof(uint8_t))
                {
                    buf.u16[index]      = uint16_t(a);
                    buf.u16[index+1]    = uint16_t(b);
                    buf.u16[index+2]    = uint16_t(c);
                    buf.u16[index+3]    = uint16_t(a);
                    buf.u16[index+4]    = uint16_t(c);
                    buf.u16[index+5]    = uint16_t(d);
                }
                else
                {
                    buf.u8[index]       = uint8_t(a);
                    buf.u8[index+1]     = uint8_t(b);
                    buf.u8[index+2]     = uint8_t(c);
                    buf.u8[index+3]     = uint8_t(a);
                    buf.u8[index+4]     = uint8_t(c);
                    buf.u8[index+5]     = uint8_t(d);
                }

                return index;
            }

            ssize_t Batch::hrectangle(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
            {
                const ssize_t index     = alloc_indices(6, d);
                if (index < 0)
                    return index;

                // Append vertex indices
                ibuffer_t & buf     = pCurrent->indices;
                if (buf.szof > sizeof(uint16_t))
                {
                    buf.u32[index]      = a;
                    buf.u32[index+1]    = b;
                    buf.u32[index+2]    = c;
                    buf.u32[index+3]    = a;
                    buf.u32[index+4]    = c;
                    buf.u32[index+5]    = d;
                }
                else if (buf.szof > sizeof(uint8_t))
                {
                    buf.u16[index]      = uint16_t(a);
                    buf.u16[index+1]    = uint16_t(b);
                    buf.u16[index+2]    = uint16_t(c);
                    buf.u16[index+3]    = uint16_t(a);
                    buf.u16[index+4]    = uint16_t(c);
                    buf.u16[index+5]    = uint16_t(d);
                }
                else
                {
                    buf.u8[index]       = uint8_t(a);
                    buf.u8[index+1]     = uint8_t(b);
                    buf.u8[index+2]     = uint8_t(c);
                    buf.u8[index+3]     = uint8_t(a);
                    buf.u8[index+4]     = uint8_t(c);
                    buf.u8[index+5]     = uint8_t(d);
                }

                return index;
            }

            ssize_t Batch::command(float **data, size_t count)
            {
                IF_DEBUG(
                    if (pCurrent == NULL)
                        return -STATUS_BAD_STATE;
                );

                const size_t to_alloc   = (count + 3) & (~size_t(3));

                // Check if we need to resize the buffer
                cbuffer_t & buf     = vCommands;
                if ((buf.count + to_alloc) > buf.capacity)
                {
                    const size_t new_cap    = buf.capacity << 2;
                    float *ptr              = static_cast<float *>(realloc(buf.data, sizeof(float) * new_cap * 4));
                    if (ptr == NULL)
                        return -STATUS_NO_MEM;

                    bzero(&ptr[buf.capacity], (new_cap - buf.capacity) * sizeof(float) * 4);

                    buf.data                = ptr;
                    buf.size              <<= 1;
                    buf.capacity            = new_cap;
                }

                // Append vertex
                const uint32_t index    = buf.count;
                buf.count              += to_alloc;

                // Clean up the unused tail
                float *result           = &buf.data[index];
                for ( ; count < to_alloc; ++count)
                    result[count]           = 0.0f;

                *data           = result;

                return index >> 2;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
