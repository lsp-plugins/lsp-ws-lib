/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 апр. 2023 г.
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

#ifndef PRIVATE_FREETYPE_LRUCACHE_H_
#define PRIVATE_FREETYPE_LRUCACHE_H_

#ifdef USE_LIBFREETYPE

#include <private/freetype/glyph.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            struct LRUCache
            {
                public:
                    glyph_t    *pHead;      // Head in the LRU cache
                    glyph_t    *pTail;      // Tail in the LRU cache

                    LRUCache();
                    ~LRUCache();

                public:
                    void         clear();
                    void         remove_glyph(glyph_t *glyph);
                    glyph_t     *remove_last();
                    glyph_t     *add_first(glyph_t *glyph);
                    glyph_t     *touch(glyph_t *glyph);
            };

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



#endif /* PRIVATE_FREETYPE_LRUCACHE_H_ */
