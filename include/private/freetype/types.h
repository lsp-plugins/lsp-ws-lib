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
#include <lsp-plug.in/lltl/phashset.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Tag structure used for in-place new operator
             */
            typedef struct allocator_tag_t
            {
            } allocator_tag_t;

            typedef uint32_t            f24p6_t;

            /**
             * The default minimum font cache size for the font manager
             */
            constexpr size_t            default_min_font_cache_size     = 8 * 1024 * 1024;

            /**
             * The default maximum font cache size for the font manager
             */
            constexpr size_t            default_max_font_cache_size     = 2 * default_min_font_cache_size;

            constexpr float             f24p6_divider           = 1.0f / 64.0f;
            constexpr float             f24p6_multiplier        = 64.0f;
            constexpr f24p6_t           f24p6_face_slant_shift  = 180; // sinf(M_PI * 9.0f / 180.0f) * 0x10000

            /**
             * The font data
             */
            typedef struct font_t
            {
                size_t          references; // Number of references
                uint8_t        *data;       // The actual data for the font stored in memory
                FT_Face         ft_face;    // The font face associated with the data
                const char     *family;     // Font family
                const char     *style;      // Font style
                const char     *name;       // The font name in the system
            } font_t;

            /**
             * Convert f24p6_t value to float
             * @param value value to convert
             * @return converted floating-point value
             */
            inline float f24p6_to_float(f24p6_t value)
            {
                return value * f24p6_divider;
            }

            /**
             * Convert float to f24p6_t value
             * @param value value to convert
             * @return converted f24p6 value
             */
            inline f24p6_t float_to_f24p6(float value)
            {
                return value * f24p6_multiplier;
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_TYPES_H_ */
