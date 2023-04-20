/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 8 апр. 2023 г.
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

#ifndef PRIVATE_FREETYPE_TYPES_H_
#define PRIVATE_FREETYPE_TYPES_H_

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/ws/ws.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Numeber of bits per pixel for a glyph
             */
            enum glyph_format_t
            {
                FMT_1_BPP       = 0,
                FMT_2_BPP       = 1,
                FMT_4_BPP       = 2,
                FMT_8_BPP       = 3,
            };

            enum face_flags_t
            {
                FACE_SLANT      = 1 << 0,
                FACE_ANTIALIAS  = 1 << 1
            };

            static constexpr float  f24p6_divider   = 1.0f / 64.0f;

            typedef uint32_t        f24p6_t;

            typedef struct face_t
            {
                size_t      references;
                FT_Face     ft_face;
                uint32_t    flags;          // Face flags
                f24p6_t     size;           // Font size
                f24p6_t     height;         // The height of the font
                f24p6_t     ascend;         // The ascender
                f24p6_t     descend;        // The descender
                f24p6_t     max_x_advance;  // The maximum X advance
            } face_t;

            /**
             * The glyph data for rendering
             */
            typedef struct glyph_t
            {
                face_t         *face;       // The pointer to the font face
                lsp_wchar_t     codepoint;  // UTF-32 codepoint associated with the glyph
                size_t          szof;       // Size of memory used for this glyph

                f24p6_t         x_advance;  // Advance by X
                f24p6_t         y_advance;  // Advance by Y
                int32_t         x_bearing;  // Bearing by X
                int32_t         y_bearing;  // Bearing by Y

                uint32_t        format;     // Bits per pixel, format
                dsp::bitmap_t   bitmap;     // The bitmap that stores the glyph data
            } glyph_t;

            /**
             * Convert f24p6_t value to float
             * @param value value to convert
             * @return converted floating-point value
             */
            inline float f24p6_to_float(f24p6_t value)
            {
                return value * f24p6_divider;
            }

            inline size_t glyph_size(const glyph_t *glyph)
            {
                return glyph->szof;
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_TYPES_H_ */
