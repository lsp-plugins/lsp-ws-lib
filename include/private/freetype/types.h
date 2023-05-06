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

            typedef int32_t             f26p6_t;

            /**
             * The default minimum font cache size for the font manager
             */
            constexpr size_t            default_min_font_cache_size     = 8 * 1024 * 1024;

            /**
             * The default maximum font cache size for the font manager
             */
            constexpr size_t            default_max_font_cache_size     = 2 * default_min_font_cache_size;

            constexpr f26p6_t           f26p6_one               = 64;
            constexpr float             f26p6_divider           = 1.0f / 64.0f;
            constexpr float             f26p6_multiplier        = 64.0f;
            constexpr f26p6_t           f26p6_face_slant_shift  = 12505; // sinf(M_PI * 12.0f / 180.0f) * 0x10000

            /**
             * The font data
             */
            typedef struct font_t
            {
                size_t          references; // Number of references
                size_t          size;       // The size of the font data
                uint8_t        *data;       // The actual data for the font stored in memory
            } font_t;

            typedef struct text_range_t
            {
                ssize_t         x_bearing;
                ssize_t         y_bearing;
                ssize_t         width;
                ssize_t         height;
                ssize_t         x_advance;
                ssize_t         y_advance;
            } text_range_t;

            /**
             * Convert f26p6_t value to float
             * @param value value to convert
             * @return converted floating-point value
             */
            LSP_HIDDEN_MODIFIER
            inline float f26p6_to_float(f26p6_t value)
            {
                return value * f26p6_divider;
            }

            /**
             * Convert f26p6_t value to float with rounding to the upper value
             * @param value value to convert
             * @return converted floating-point value
             */
            LSP_HIDDEN_MODIFIER
            inline ssize_t f26p6_ceil_to_int(f26p6_t value)
            {
                return (value + f26p6_one - 1) / f26p6_one;
            }

            /**
             * Convert f26p6_t value to float with rounding to the lower value
             * @param value value to convert
             * @return converted floating-point value
             */
            LSP_HIDDEN_MODIFIER
            inline ssize_t f26p6_floor_to_int(f26p6_t value)
            {
                return value / f26p6_one;
            }

            /**
             * Convert float to f26p6_t value
             * @param value value to convert
             * @return converted f26p6 value
             */
            LSP_HIDDEN_MODIFIER
            inline f26p6_t float_to_f26p6(float value)
            {
                return value * f26p6_multiplier;
            }

            /**
             * Convert float to f26p6_t value
             * @param value value to convert
             * @return converted f26p6 value
             */
            LSP_HIDDEN_MODIFIER
            inline f26p6_t int_to_f26p6(ssize_t value)
            {
                return value * f26p6_one;
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_TYPES_H_ */
