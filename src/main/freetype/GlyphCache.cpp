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

#ifdef USE_LIBFREETYPE

#include <private/freetype/GlyphCache.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            GlyphCache::GlyphCache()
            {
                nSize       = 0;
                nCap        = 0;
                vBins       = NULL;
            }

            GlyphCache::~GlyphCache()
            {
                nSize       = 0;
                nCap        = 0;
                if (vBins != NULL)
                {
                    free(vBins);
                    vBins       = NULL;
                }
            }

            bool GlyphCache::put(glyph_t *glyph)
            {
                bin_t *bin      = (vBins != NULL) ? &vBins[glyph->codepoint & (nCap - 1)] : NULL;
                if (bin != NULL)
                {
                    // Ensure that glyph is not present
                    for (glyph_t *g = bin->data; g != NULL; g = g->cache_next)
                        if (g->codepoint == glyph->codepoint)
                            return false;
                }

                // Need to grow?
                if (nSize >= (nCap << 2))
                {
                    if (!grow())
                        return false;
                    bin      = (vBins != NULL) ? &vBins[glyph->codepoint & (nCap - 1)] : NULL;
                }

                // Add item to the bin
                glyph->cache_next   = bin->data;
                bin->data           = glyph;
                ++bin->size;
                ++nSize;

                return true;
            }

            glyph_t *GlyphCache::get(lsp_wchar_t codepoint)
            {
                bin_t *bin      = (vBins != NULL) ? &vBins[codepoint & (nCap - 1)] : NULL;
                if (bin == NULL)
                    return NULL;

                // Find the corresponding glyph
                for (glyph_t *g = bin->data; g != NULL; g = g->cache_next)
                    if (g->codepoint == codepoint)
                        return g;

                return NULL;
            }

            bool GlyphCache::remove(glyph_t *glyph)
            {
                // Get the bin
                bin_t *bin      = (vBins != NULL) ? &vBins[glyph->codepoint & (nCap - 1)] : NULL;
                if (bin == NULL)
                    return false;

                // Scan the list
                for (glyph_t **pcurr = &bin->data; *pcurr != NULL; )
                {
                    glyph_t *curr = *pcurr;
                    if (curr->codepoint == glyph->codepoint)
                    {
                        *pcurr              = curr->cache_next;
                        curr->cache_next    = NULL;
                        --bin->size;
                        --nSize;
                        return true;
                    }
                    pcurr   = &curr->cache_next;
                }

                return false;
            }

            glyph_t *GlyphCache::clear()
            {
                if (vBins == NULL)
                    return NULL;

                glyph_t *root   = NULL;

                for (size_t i=0; i<nCap; ++i)
                {
                    // Get the next bin, ensure that it contains data
                    bin_t *bin      = &vBins[i];
                    if (bin->data == NULL)
                        continue;

                    // Walk through the list and find last item
                    glyph_t *curr       = bin->data;
                    while (curr->cache_next != NULL)
                        curr    = curr->cache_next;

                    // Link the list of removed glyphs
                    curr->cache_next    = root;
                    root                = bin->data;
                }

                // Cleanup all data
                nSize       = 0;
                nCap        = 0;
                if (vBins != NULL)
                {
                    free(vBins);
                    vBins       = NULL;
                }

                return root;
            }

            bool GlyphCache::grow()
            {
                bin_t *xbin, *ybin;
                size_t ncap, mask;

                // No previous allocations?
                if (nCap == 0)
                {
                    xbin            = static_cast<bin_t *>(::malloc(0x10 * sizeof(bin_t)));
                    if (xbin == NULL)
                        return false; // Very bad things?

                    nCap            = 0x10;
                    vBins           = xbin;
                    for (size_t i=0; i<nCap; ++i, ++xbin)
                    {
                        xbin->size      = 0;
                        xbin->data      = NULL;
                    }

                    return true;
                }

                // Twice increase the capacity of hash
                ncap            = nCap << 1;
                xbin            = static_cast<bin_t *>(::realloc(vBins, ncap * sizeof(bin_t)));
                if (xbin == NULL)
                    return false; // Very bad things?

                // Now we need to split data
                mask            = (ncap - 1) ^ (nCap - 1); // mask indicates the bit which has been set
                vBins           = xbin;
                ybin            = &xbin[nCap];

                for (size_t i=0; i<nCap; ++i, ++xbin, ++ybin)
                {
                    // There is no data in ybin by default
                    ybin->size      = 0;
                    ybin->data      = 0;

                    // Migrate items from xbin list to ybin list
                    for (glyph_t **pcurr = &xbin->data; *pcurr != NULL; )
                    {
                        glyph_t *curr   = *pcurr;
                        if (curr->codepoint & mask)
                        {
                            *pcurr              = curr->cache_next;
                            curr->cache_next    = ybin->data;
                            ybin->data          = curr;
                            --xbin->size;
                            ++ybin->size;
                        }
                        else
                            pcurr               = &curr->cache_next;
                    }
                }

                // Split success
                nCap            = ncap;

                return true;
            }


        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */


