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

#include <private/freetype/types.h>

#ifdef USE_LIBFREETYPE

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Make glyph from the freetype glyph slot
             * @param face the font face to make the glyph
             * @param ch UTF-32 codepoint to make the glyph
             * @return pointer to allocated glyph or NULL if no memory is available
             */
            glyph_t *make_glyph(face_t *face, lsp_wchar_t ch);

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
