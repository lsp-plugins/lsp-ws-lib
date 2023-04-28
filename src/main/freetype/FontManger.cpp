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
#include <lsp-plug.in/io/InFileStream.h>

#include <private/freetype/FontManager.h>
#include <private/freetype/FontSpec.h>
#include <private/freetype/face.h>
#include <private/freetype/glyph.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            FontManager::FontManager():
                vFaces(),
                vFontCache(
                    font_hash_iface(),
                    font_compare_iface(),
                    font_allocator_iface())
            {
                hLibrary        = NULL;
                nCacheSize      = 0;
                nMinCacheSize   = default_min_font_cache_size;
                nMaxCacheSize   = default_max_font_cache_size;
            }

            FontManager::~FontManager()
            {
                destroy();
            }

            status_t FontManager::init()
            {
                if (hLibrary != NULL)
                    return STATUS_BAD_STATE;

                FT_Error error;
                if ((error = FT_Init_FreeType(&hLibrary)) != FT_Err_Ok)
                    return STATUS_UNKNOWN_ERR;

                return STATUS_OK;
            }

            void FontManager::destroy()
            {
                clear();

                if (hLibrary != NULL)
                {
                    FT_Done_FreeType(hLibrary);
                    hLibrary    = NULL;
                }
            }

            void FontManager::dereference(face_t *face)
            {
                if (face == NULL)
                    return;
                if ((--face->references) <= 0)
                    destroy_face(face);
            }

            bool FontManager::add_font_face(lltl::darray<font_entry_t> *entries, const char *name, face_t *face)
            {
                // Add font entry by font's family name
                font_entry_t *entry = entries->add();
                if (entry == NULL)
                    return false;

                entry->name         = NULL;
                entry->face         = face;
                ++face->references;

                // Copy face name
                if ((entry->name = strdup(name)) == NULL)
                    return false;

                return true;
            }

            status_t FontManager::add_font(const char *name, const char *path)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                io::InFileStream ifs;
                status_t res    = ifs.open(path);
                if (res == STATUS_OK)
                    res         = add(name, &ifs);
                status_t res2   = ifs.close();
                return (res == STATUS_OK) ? res2 : res;
            }

            status_t FontManager::add_font(const char *name, const io::Path *path)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                io::InFileStream ifs;
                status_t res    = ifs.open(path);
                if (res == STATUS_OK)
                    res         = add(name, &ifs);
                status_t res2   = ifs.close();
                return (res == STATUS_OK) ? res2 : res;
            }

            status_t FontManager::add_font(const char *name, const LSPString *path)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                io::InFileStream ifs;
                status_t res    = ifs.open(path);
                if (res == STATUS_OK)
                    res         = add(name, &ifs);
                status_t res2   = ifs.close();
                return (res == STATUS_OK) ? res2 : res;
            }

            status_t FontManager::add(const char *name, io::IInStream *is)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

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
                if (!vFaces.insert(0, &entries))
                    return STATUS_NO_MEM;

                // Invalidate the face cache entries and update mapping
                for (size_t i=0, n=entries.size(); i<n; ++i)
                {
                    font_entry_t *entry = entries.uget(i);
                    if (entry != NULL)
                        invalidate_faces(entry->name);
                }

                // Prevent from destroying temporary data structures
                entries.flush();
                faces.flush();

                return STATUS_OK;
            }

            status_t FontManager::add_alias(const char *name, const char *alias)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                // Check that alias does not exists
                if (vAliases.get(name) != NULL)
                    return STATUS_ALREADY_EXISTS;

                // Ensure that there is no such font
                for (size_t i=0, n=vFaces.size(); i<n; ++i)
                {
                    font_entry_t *fe = vFaces.uget(i);
                    if ((fe != NULL) && (strcmp(fe->name, name) == 0))
                        return STATUS_ALREADY_EXISTS;
                }

                // Copy the alias name
                char *aliased = strdup(alias);
                if (aliased == NULL)
                    return STATUS_NO_MEM;

                // Allocate place in the alias list
                if (!vAliases.create(name, aliased))
                    return STATUS_NO_MEM;

                return STATUS_OK;
            }

            void FontManager::invalidate_face(face_t *face)
            {
                // Obtain the list of all glyphs in the face
                lltl::parray<glyph_t> glyphs;
                if (!face->cache.values(&glyphs))
                    return;
                lsp_finally { face->cache.flush(); };

                // Remove all glyphs from LRU cache
                for (size_t i=0, n=glyphs.size(); i<n; ++i)
                {
                    glyph_t *glyph  = glyphs.uget(i);
                    if (glyph != NULL)
                    {
                        sLRU.remove(glyph);
                        free_glyph(glyph);
                    }
                }

                // Update counters
                nCacheSize         -= face->cache_size;
                face->cache_size    = 0;
            }

            void FontManager::invalidate_faces(const char *name)
            {
                // Ensure that the argument is correct
                if (name == NULL)
                    return;

                // Obtain the list of fonts and faces
                lltl::parray<Font> fonts;
                if (!vFontCache.keys(&fonts))
                    return;

                // Remove all elements with the same font name
                face_t *face = NULL;
                for (size_t i=0, n=fonts.size(); i<n; ++i)
                {
                    Font *f = fonts.uget(i);
                    if ((f != NULL) && (strcmp(f->name(), name) == 0))
                    {
                        if (vFontCache.remove(f, &face))
                        {
                            // Invalidate face and dereference it
                            invalidate_face(face);
                            dereference(face);
                        }
                    }
                }
            }

            status_t FontManager::remove(const char *name)
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                // Step 1: Find alias and remove it
                char *alias = NULL;
                if (vAliases.remove(name, &alias))
                {
                    free(alias);
                    return STATUS_OK;
                }

                // Step 2: Find font face with corresponding name
                font_entry_t *fe    = NULL;
                for (size_t i=0, n=vFaces.size(); i<n; ++i)
                {
                    font_entry_t *entry = vFaces.uget(i);
                    if ((entry != NULL) && (strcmp(entry->name, name) == 0))
                    {
                        fe              = entry;
                        break;
                    }
                }
                if (fe == NULL)
                    return STATUS_NOT_FOUND;

                // Step 3: Drop all entries with the same font face
                face_t *face        = fe->face;
                for (size_t i=0; i<vFaces.size();)
                {
                    // Get the item
                    font_entry_t *entry = vFaces.uget(i);
                    if (entry->face != face)
                    {
                        ++i;
                        continue;
                    }

                    // Invalidate the corresponding font face
                    invalidate_faces(entry->name);

                    // Remove the entry
                    if (entry->name != NULL)
                        free(entry->name);
                    dereference(face);
                    vFaces.remove(i);
                }

                return STATUS_OK;
            }

            status_t FontManager::clear()
            {
                if (hLibrary == NULL)
                    return STATUS_BAD_STATE;

                // Invalidate all faces
                lltl::parray<face_t> fonts;
                if (!vFontCache.values(&fonts))
                    return STATUS_NO_MEM;
                vFontCache.flush();

                for (size_t i=0, n=fonts.size(); i<n; ++i)
                {
                    face_t *face = fonts.uget(i);
                    if (face != NULL)
                    {
                        if ((--face->references) <= 0)
                            destroy_face(face);
                    }
                }
                fonts.flush();

                // Cleanup all fonts
                for (size_t i=0, n=vFaces.size(); i<n; ++i)
                {
                    font_entry_t *entry = vFaces.uget(i);
                    if (entry != NULL)
                    {
                        if (entry->name != NULL)
                            free(entry->name);
                        if ((--entry->face->references) <= 0)
                            destroy_face(entry->face);
                    }
                }
                vFaces.flush();

                return STATUS_OK;
            }

            void FontManager::gc()
            {
                if (hLibrary == NULL)
                    return;

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



