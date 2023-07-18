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

#ifdef USE_LIBFREETYPE

#include <private/freetype/LRUCache.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            LRUCache::LRUCache()
            {
                pHead       = NULL;
                pTail       = NULL;
            }

            LRUCache::~LRUCache()
            {
                pHead       = NULL;
                pTail       = NULL;
            }

            void LRUCache::clear()
            {
                pHead       = NULL;
                pTail       = NULL;
            }

            void LRUCache::remove(glyph_t *glyph)
            {
                if (glyph->lru_prev != NULL)
                    glyph->lru_prev->lru_next   = glyph->lru_next;
                else
                    pHead               = glyph->lru_next;
                if (glyph->lru_next != NULL)
                    glyph->lru_next->lru_prev   = glyph->lru_prev;
                else
                    pTail               = glyph->lru_prev;

                // Clear links to other glyphs
                glyph->lru_prev     = NULL;
                glyph->lru_next     = NULL;
            }

            glyph_t *LRUCache::remove_last()
            {
                if (pTail == NULL)
                    return NULL;

                // Remove glyph from the LRU list
                glyph_t *glyph  = pTail;

                pTail           = glyph->lru_prev;
                if (pTail != NULL)
                    pTail->lru_next     = NULL;
                else
                    pHead               = NULL;

                // Clear links to other glyphs
                glyph->lru_prev     = NULL;
                glyph->lru_next     = NULL;

                return glyph;
            }

            glyph_t *LRUCache::add_first(glyph_t *glyph)
            {
                if (pHead != NULL)
                {
                    glyph->lru_next     = pHead;
                    glyph->lru_prev     = NULL;
                    pHead->lru_prev     = glyph;
                    pHead               = glyph;
                    return glyph;
                }

                glyph->lru_next     = NULL;
                glyph->lru_prev     = NULL;
                pHead               = glyph;
                pTail               = glyph;

                return glyph;
            }

            glyph_t *LRUCache::touch(glyph_t *glyph)
            {
                // Remove glyph from the list
                if (glyph->lru_prev != NULL)
                    glyph->lru_prev->lru_next   = glyph->lru_next;
                else
                    return glyph;

                if (glyph->lru_next != NULL)
                    glyph->lru_next->lru_prev   = glyph->lru_prev;
                else
                    pTail               = glyph->lru_prev;

                // Add glyph to the head
                glyph->lru_next     = pHead;
                glyph->lru_prev     = NULL;
                pHead->lru_prev     = glyph;
                pHead           = glyph;

                return glyph;
            }

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */


