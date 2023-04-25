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

#ifdef USE_LIBFREETYPE

#include <private/freetype/FontManager.h>
#include <private/freetype/FontSpec.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            FontManager::FontManager(FT_Library library):
                vCustomFonts(),
                vFontMapping(
                    font_hash_iface(),
                    font_compare_iface(),
                    font_allocator_iface())
            {
                hLibrary        = library;
                nCacheSize      = 0;
                nMinCacheSize   = default_min_font_cache_size;
                nMaxCacheSize   = default_max_font_cache_size;
            }

            FontManager::~FontManager()
            {
                clear();
            }

            status_t FontManager::add_font(const char *name, io::IInStream *is)
            {
                // TODO
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t FontManager::add_font_alias(const char *name, const char *alias)
            {
                // TODO
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t FontManager::remove_font(const char *name)
            {
                // TODO
                return STATUS_NOT_IMPLEMENTED;
            }

            void FontManager::clear()
            {
                // TODO
            }

            void FontManager::gc()
            {
                while (nCacheSize <= nMaxCacheSize)
                {
                    // Remove glyph from LRU cache
                    glyph_t *glyph      = sLRU.remove_last();
                    if (glyph == NULL)
                        break;

                    // Remove glyph from the face's cache
                    face_t *face        = glyph->face;
                    if (face->cache.remove(glyph, &glyph))
                    {
                        face->cache_size   -= glyph->szof;
                        nCacheSize         -= glyph->szof;
                    }

                    // Free the glyph data
                    free_glyph(glyph);
                }
            }

            glyph_t *FontManager::get_glyph(face_t *face, lsp_wchar_t ch)
            {
                glyph_t key;
                key.codepoint   = ch;

                // Try to obtain glyph from cache
                glyph_t *glyph  = face->cache.get(&key);
                if (glyph != NULL)
                    return sLRU.touch(glyph); // Move glyph to the beginning of the LRU cache

                // There was no glyph present, create new glyph
                glyph           = render_glyph(face, ch);
                if (glyph == NULL)
                    return NULL;

                // Add glyph to the face cache
                if (face->cache.create(glyph))
                {
                    // Call the garbage collector to collect the garbage
                    gc();

                    // Update cache statistics
                    face->cache_size   += glyph->szof;
                    nCacheSize         += glyph->szof;

                    // Add glyph to the LRU cache
                    return sLRU.add_first(glyph);
                }

                // Failed to add glyph
                free_glyph(glyph);
                return NULL;
            }

            void FontManager::set_cache_limits(size_t min, size_t max)
            {
                size_t old_size             = nMaxCacheSize;
                nMinCacheSize               = min;
                nMaxCacheSize               = max;

                if (nMaxCacheSize < old_size)
                    gc();
            }

            size_t FontManager::set_min_cache_size(size_t min)
            {
                size_t old_size             = nMinCacheSize;
                nMinCacheSize               = min;
                return old_size;
            }

            size_t FontManager::set_max_cache_size(size_t max)
            {
                size_t old_size             = nMaxCacheSize;
                nMaxCacheSize               = max;
                if (nMaxCacheSize < old_size)
                    gc();
                return old_size;
            }

            size_t FontManager::min_cache_size() const
            {
                return nMinCacheSize;
            }

            size_t FontManager::max_cache_size() const
            {
                return nMaxCacheSize;
            }

            size_t FontManager::used_cache_size() const
            {
                return nCacheSize;
            }

            bool FontManager::get_font_parameters(const Font *f, font_parameters_t *fp)
            {
                // TODO
                return false;
            }

            bool FontManager::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                // TODO
                return false;
            }

            dsp::bitmap_t *FontManager::render_text(const LSPString *text, size_t first, size_t last)
            {
                // TODO
                return NULL;
            }
        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



