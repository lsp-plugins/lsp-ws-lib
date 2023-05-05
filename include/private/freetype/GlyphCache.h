/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 2 мая 2023 г.
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

#ifndef PRIVATE_FREETYPE_GLYPHCACHE_H_
#define PRIVATE_FREETYPE_GLYPHCACHE_H_

#ifdef USE_LIBFREETYPE

#include <private/freetype/glyph.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            class GlyphCache
            {
                protected:
                    typedef struct bin_t
                    {
                        size_t      size;       // Number of used tuples in storage
                        glyph_t    *data;       // List of glyphs
                    } bin_t;

                protected:
                    size_t          nSize;      // Overall size of the hash
                    size_t          nCap;       // Capacity in bins
                    bin_t          *vBins;      // Overall array of bins

                protected:
                    bool            grow();

                public:
                    GlyphCache();
                    ~GlyphCache();

                public:
                    glyph_t        *clear();
                    bool            put(glyph_t *glyph);
                    bool            remove(glyph_t *glyph);
                    glyph_t        *get(lsp_wchar_t codepoint);

                    inline size_t   size() const        { return nSize; }
                    inline size_t   capacity() const    { return nCap;  }
            };

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */




#endif /* PRIVATE_FREETYPE_GLYPHCACHE_H_ */
