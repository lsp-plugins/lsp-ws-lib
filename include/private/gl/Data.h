/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 2 июн. 2025 г.
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

#ifndef PRIVATE_GL_DATA_H_
#define PRIVATE_GL_DATA_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class Texture;

            enum program_t
            {
                GEOMETRY,
                STENCIL,
            };

            enum attribute_t
            {
                VERTEX_COORDS,
                TEXTURE_COORDS,
                COMMAND_BUFFER,
            };

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

            enum texture_format_t
            {
                TEXTURE_UNKNOWN     = -1,    // No texture format specified
                TEXTURE_RGBA32      = 0,     // 8-bit alpha component
                TEXTURE_ALPHA8      = 1,     // 8-bit alpha component
                TEXTURE_PRGBA32     = 2,     // 32-bit color with 8 bits per red, gree, blue and alpha components, alpha is premultiplied
            };

            enum index_format_t
            {
                INDEX_FMT_U8,
                INDEX_FMT_U16,
                INDEX_FMT_U32,
            };

            typedef struct LSP_HIDDEN_MODIFIER uniform_t
            {
                const char         *name;
                uniform_type_t      type;
                union {
                    const void         *raw;
                    const GLfloat      *f32;
                    const GLint        *i32;
                    const GLuint       *u32;
                };
            } uniform_t;

            typedef struct LSP_HIDDEN_MODIFIER vertex_t
            {
                float               x;      // X Coordinate
                float               y;      // Y Coordinate
                float               s;      // Texture Coordinate S
                float               t;      // Texture Coordinate T
                uint32_t            cmd;    // Draw command
            } vertex_t;

            typedef struct LSP_HIDDEN_MODIFIER batch_vbuffer_t
            {
                vertex_t           *v;
                uint32_t            count;
                uint32_t            capacity;
            } batch_vbuffer_t;

            typedef struct LSP_HIDDEN_MODIFIER batch_ibuffer_t
            {
                union {
                    uint8_t            *u8;
                    uint16_t           *u16;
                    uint32_t           *u32;
                    void               *data;
                };

                uint32_t            count;
                uint32_t            capacity;
                uint32_t            szof;
            } batch_ibuffer_t;

            typedef struct LSP_HIDDEN_MODIFIER batch_cbuffer_t
            {
                float              *data;       // Pointer to actual data
                uint32_t            count;      // Number of filled floats (should be always multiple of 4)
                uint32_t            size;       // Texture size
                uint32_t            capacity;   // Overall capacity in RGBAF32 components (4 floats per record)
            } batch_cbuffer_t;

            typedef struct LSP_HIDDEN_MODIFIER batch_header_t
            {
                gl::program_t       enProgram;  // Used program for rendering
                int32_t             nLeft;      // Origin offset left
                int32_t             nTop;       // Origin offset top
                uint32_t            nFlags;     // Flags
                gl::Texture        *pTexture;   // Related texture
            } batch_header_t;

            typedef struct LSP_HIDDEN_MODIFIER batch_draw_t
            {
                batch_header_t      header;
                batch_vbuffer_t     vertices;
                batch_ibuffer_t     indices;
                batch_draw_t       *next;
                uint32_t            ttl;
            } batch_draw_t;


            template <class T>
            T *safe_acquire(T *ptr)
            {
                if (ptr != NULL)
                    ptr->reference_up();
                return ptr;
            }

            template <class T>
            void safe_release(T * &ptr)
            {
                if (ptr == NULL)
                    return;

                ptr->reference_down();
                ptr = NULL;
            }
        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_DATA_H_ */
