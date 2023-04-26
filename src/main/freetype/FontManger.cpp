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

#include <lsp-plug.in/common/debug.h>

#include <private/freetype/FontManager.h>
#include <private/freetype/FontSpec.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            FontManager::FontManager(FT_Library library):
                vLoadedFaces(),
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

            bool FontManager::add_font_face(lltl::darray<font_entry_t> *entries, const char *name, face_t *face)
            {
                // Add font entry by font's family name
                font_entry_t *entry = entries->add();
                if (entry == NULL)
                    return false;

                entry->name         = NULL;
                entry->face         = face;
                entry->aliased      = NULL;
                ++face->references;

                // Copy face name
                if ((entry->name = strdup(name)) == NULL)
                    return false;

                return true;
            }

            status_t FontManager::add_font(const char *name, io::IInStream *is)
            {
                status_t res;
                lltl::parray<face_t> faces;

                // Load font faces
                if ((res = load_face(&faces, hLibrary, is)) != STATUS_OK)
                    return res;
                lsp_finally { destroy_faces(&faces); };

                // Make list of faces
                lltl::darray<font_entry_t> entries;
                if (!entries.reserve(faces.size() + 1))
                    return STATUS_NO_MEM;
                lsp_finally {
                    for (size_t i=0, n=entries.size(); i<n; ++i)
                    {
                        font_entry_t *e = entries.uget(i);
                        if (e == NULL)
                            continue;
                        if (e->name != NULL)
                            free(e->name);
                        if (e->aliased != NULL)
                            free(e->aliased);
                    }
                    entries.flush();
                };

                // Create font entries
                for (size_t i=0, n=faces.size(); i<n; ++i)
                {
                    face_t *face        = faces.uget(i);

                    // Add font entry by font's family name
                    if (!add_font_face(&entries, face->ft_face->family_name, face))
                        return STATUS_NO_MEM;

                    // Add font entry by font's name
                    if ((i == 0) && (name != NULL))
                    {
                        if (!add_font_face(&entries, name, face))
                            return STATUS_NO_MEM;
                    }
                }

                // Deploy loaded font entries to the list of loaded faces
                if (!vLoadedFaces.insert(0, &entries))
                    return STATUS_NO_MEM;

                // Invalidate the face cache entries
                for (size_t i=0, n=entries.size(); i<n; ++i)
                {
                    font_entry_t *entry = entries.uget(i);
                    if (entry != NULL)
                        invalidate_face(entry->name);
                }

                // Prevent from destroying temporary data structures
                entries.flush();
                faces.flush();

                return STATUS_OK;
            }

            status_t FontManager::add_font_alias(const char *name, const char *alias)
            {
                // Check that alias not exists
                for (size_t i=0, n=vLoadedFaces.size(); i<n; ++i)
                {
                    font_entry_t *fe = vLoadedFaces.uget(i);
                    if ((fe != NULL) && (strcmp(fe->name, name) == 0))
                        return STATUS_ALREADY_EXISTS;
                }

                // Create new entry
                font_entry_t entry;
                entry.name      = NULL;
                entry.face      = NULL;
                entry.aliased   = NULL;
                lsp_finally {
                    if (entry.name != NULL)
                        free(entry.name);
                    if (entry.aliased != NULL)
                        free(entry.aliased);
                };

                // Copy name and alias
                if ((entry.name = strdup(name)) == NULL)
                    return STATUS_NO_MEM;
                if ((entry.aliased = strdup(alias)) == NULL)
                    return STATUS_NO_MEM;

                // Allocate place in the font list
                if (!vLoadedFaces.insert(0, entry))
                    return STATUS_NO_MEM;

                // Invalidate face
                invalidate_face(entry.name);

                // Commit result and return
                entry.name      = NULL;
                entry.aliased   = NULL;

                return STATUS_OK;
            }

            void FontManager::invalidate_face(const char *name)
            {
                if (name == NULL)
                    return;

                // TODO: remove all records from the cache with the same face name
            }

            status_t FontManager::remove_font(const char *name)
            {
                // Step 1: Find font face with corresponding name
                font_entry_t *fe    = NULL;
                for (size_t i=0, n=vLoadedFaces.size(); i<n; ++i)
                {
                    font_entry_t *entry = vLoadedFaces.uget(i);
                    if ((entry != NULL) && (strcmp(entry->name, name) == 0))
                    {
                        fe              = entry;
                        break;
                    }
                }
                if (fe == NULL)
                    return STATUS_NOT_FOUND;

                // Step 2: just remove entry if it is an alias
                if (fe->face == NULL)
                {
                    // Drop the entry and exit
                    free(fe->name);
                    if (fe->aliased == NULL)
                        free(fe->aliased);
                    vLoadedFaces.premove(fe);
                    return STATUS_OK;
                }

                // Invalidate the corresponding font face
                invalidate_face(fe->name);

                // Drop all entries with the same font face
                face_t *face        = fe->face;
                for (size_t i=0, n=vLoadedFaces.size(); i<n;)
                {
                    // Get the item
                    font_entry_t *entry = vLoadedFaces.uget(i);
                    if (entry->face != face)
                    {
                        ++i;
                        continue;
                    }

                    // Remove the entry
                    if (entry->name != NULL)
                        free(entry->name);
                    if (entry->aliased != NULL)
                        free(entry->aliased);
                    --face->references;
                    vLoadedFaces.remove(i);
                }

                // Destroy the corresponding face
                if (face->references > 0)
                {
                    lsp_error("Malformed face state");
                    return STATUS_CORRUPTED;
                }
                destroy_face(face);

                return STATUS_OK;
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



