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

#ifndef PRIVATE_FREETYPE_GLYPH_H_
#define PRIVATE_FREETYPE_GLYPH_H_

#ifdef USE_LIBFREETYPE

#include <private/freetype/types.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            struct face_t;

            /**
             * Number of bits per pixel for a glyph
             */
            enum glyph_format_t
            {
                FMT_1_BPP       = 0,
                FMT_2_BPP       = 1,
                FMT_4_BPP       = 2,
                FMT_8_BPP       = 3,
            };

            /**
             * The glyph data for rendering
             */
            typedef struct glyph_t
            {
                glyph_t        *cache_next; // The pointer to the next item in the hash
                glyph_t        *lru_next;   // Pointer to next glyph in the LRU cache
                glyph_t        *lru_prev;   // Pointer to previous glyph in the LRU cache

                face_t         *face;       // The pointer to the font face
                lsp_wchar_t     codepoint;  // UTF-32 codepoint associated with the glyph
                size_t          szof;       // Size of memory used for this glyph

                f26p6_t         width;      // Glyph width
                f26p6_t         height;     // Glyph height
                f26p6_t         x_advance;  // Advance by X
                f26p6_t         y_advance;  // Advance by Y
                int32_t         x_bearing;  // Bearing by X
                int32_t         y_bearing;  // Bearing by Y

                uint32_t        format;     // Bits per pixel, format
                dsp::bitmap_t   bitmap;     // The bitmap that stores the glyph data
            } glyph_t;

            /**
             * Use the font face to load glyph and render it
             * @param face the font face to load the glyph
             * @param ch UTF-32 codepoint of the glyph
             * @return pointer to allocated glyph or NULL if no memory is available
             */
            glyph_t *render_glyph(face_t *face, lsp_wchar_t ch);

            /**
             * Free the glyph and data associated with it
             * @param glyph glyph to destroy
             */
            void free_glyph(glyph_t *glyph);

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_GLYPH_H_ */
