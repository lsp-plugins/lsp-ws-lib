/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 23 апр. 2023 г.
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

#ifndef PRIVATE_FREETYPE_FACE_H_
#define PRIVATE_FREETYPE_FACE_H_

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/ws/ws.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/phashset.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <private/freetype/types.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            struct glyph_t;

            enum face_flags_t
            {
                FACE_SLANT      = 1 << 0,
                FACE_BOLD       = 1 << 1,
                FACE_ANTIALIAS  = 1 << 2,
                FACE_SYNTHETIC  = 1 << 3
            };

            /**
             * The font face
             */
            typedef struct face_t
            {
                size_t      references;         // Number of references
                size_t      cache_size;         // The amount of memory used by glyphs in cache
                FT_Face     ft_face;            // The font face
                font_t     *font;               // The font data

                uint32_t    flags;              // Face flags
                f24p6_t     h_size;             // The horizontal character size
                f24p6_t     v_size;             // The verical character size
                f24p6_t     height;             // The height of the font
                f24p6_t     ascend;             // The ascender
                f24p6_t     descend;            // The descender

                lltl::phashset<glyph_t> cache;  // Glyph cache
            } face_t;

            /**
             * Make font face flags
             * @param f font specification
             * @return font face flags
             */
            size_t      make_face_flags(const Font *f);

            /**
             * Load font face
             * @param faces array to store all loaded font faces
             * @param ft the FreeType library handle
             * @param is input stream with the font data
             * @return status of operation
             */
            status_t    load_face(lltl::parray<face_t> *faces, FT_Library ft, io::IInStream *is);

            /**
             * Create font face
             * @param ft_face freetype font face to use as a reference
             * @param flags font face flags
             * @return pointer to font face
             */
            face_t     *clone_face(face_t *src, uint32_t flags);

            /**
             * Destroy the font face
             * @param face the font face to destroy
             */
            void        destroy_face(face_t *face);

            /**
             * Destroy the list of font faces
             * @param face the font face to destroy
             */
            void        destroy_faces(lltl::parray<face_t> *faces);

            /**
             * Start text processing using the selected face
             * @param face face object
             * @param size the font size of the face
             * @return status of operation
             */
            status_t    select_face(face_t *face, float size);

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */

#endif /* PRIVATE_FREETYPE_FACE_H_ */
