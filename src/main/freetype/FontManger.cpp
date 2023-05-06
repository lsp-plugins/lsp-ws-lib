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
#include <private/freetype/bitmap.h>
#include <private/freetype/face_id.h>
#include <private/freetype/face.h>
#include <private/freetype/glyph.h>
#include <private/freetype/types.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            FontManager::FontManager()
            {
                hLibrary        = NULL;

                nCacheSize      = 0;
                nMinCacheSize   = default_min_font_cache_size;
                nMaxCacheSize   = default_max_font_cache_size;
                nFaceHits       = 0;
                nFaceMisses     = 0;
                nGlyphHits      = 0;
                nGlyphMisses    = 0;
                nGlyphRemoval   = 0;
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
                if (hLibrary == NULL)
                    return;

                // Output cache statistics
                lsp_info("Cache statistics:");
                lsp_info("  Memory:         %ld", long(nCacheSize));
                lsp_info("  Face hits:      %ld", long(nFaceHits));
                lsp_info("  Face misses:    %ld", long(nFaceMisses));
                lsp_info("  Glyph hits:     %ld", long(nGlyphHits));
                lsp_info("  Glyph misses:   %ld", long(nGlyphMisses));
                lsp_info("  Glyph removal:  %ld", long(nGlyphRemoval));

                // Destroy the state
                clear();
                clear_cache_stats();

                FT_Done_FreeType(hLibrary);
                hLibrary    = NULL;
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

            status_t FontManager::add(const char *name, const char *path)
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

            status_t FontManager::add(const char *name, const io::Path *path)
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

            status_t FontManager::add(const char *name, const LSPString *path)
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
                if (face == NULL)
                    return;

                // Remove all glyphs from LRU cache
                glyph_t *glyph = face->cache.clear();
                while (glyph != NULL)
                {
                    glyph_t *next   = glyph->cache_next;
                    sLRU.remove(glyph);
                    free_glyph(glyph);
                    glyph           = next;
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
                lltl::parray<face_id_t> face_ids;
                if (!vFontCache.keys(&face_ids))
                    return;

                // Remove all elements with the same font name
                face_t *face = NULL;
                for (size_t i=0, n=face_ids.size(); i<n; ++i)
                {
                    face_id_t *f = face_ids.uget(i);
                    if ((f != NULL) && (strcmp(f->name, name) == 0))
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
                    dereference(face);
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
                        dereference(entry->face);
                    }
                }
                vFaces.flush();

                // Remove all aliases
                lltl::parray<char> va;
                if (vAliases.values(&va))
                {
                    for (size_t i=0, n=va.size(); i<n; ++i)
                    {
                        char *str = va.uget(i);
                        if (str != NULL)
                            free(str);
                    }
                }
                vAliases.flush();

                return STATUS_OK;
            }

            void FontManager::gc()
            {
                if (hLibrary == NULL)
                    return;
                if (nCacheSize <= nMaxCacheSize)
                    return;
                size_t cache_size = lsp_min(nMinCacheSize, nMaxCacheSize);

                while (nCacheSize > cache_size)
                {
                    // Remove glyph from LRU cache
                    glyph_t *glyph      = sLRU.remove_last();
                    if (glyph == NULL)
                        break;

                    // Remove glyph from the face's cache
                    face_t *face        = glyph->face;
                    if (face->cache.remove(glyph))
                    {
                        ++nGlyphRemoval;
                        face->cache_size   -= glyph->szof;
                        nCacheSize         -= glyph->szof;
                    }

                    // Free the glyph data
                    free_glyph(glyph);
                }
            }

            glyph_t *FontManager::get_glyph(face_t *face, lsp_wchar_t ch)
            {
                // Try to obtain glyph from cache
                glyph_t *glyph  = face->cache.get(ch);
                if (glyph != NULL)
                {
                    ++nGlyphHits;
                    return sLRU.touch(glyph); // Move glyph to the beginning of the LRU cache
                }
                ++nGlyphMisses;

                // There was no glyph present, create new glyph
                glyph           = render_glyph(hLibrary, face, ch);
                if (glyph == NULL)
                    return NULL;

                // Add glyph to the face cache
                if (face->cache.put(glyph))
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

            void FontManager::clear_cache_stats()
            {
                nFaceHits                   = 0;
                nFaceMisses                 = 0;
                nGlyphHits                  = 0;
                nGlyphMisses                = 0;
                nGlyphRemoval               = 0;
            }

            face_t *FontManager::find_face(const face_id_t *id)
            {
                size_t selector = id->flags & (FID_BOLD | FID_ITALIC);

                for (size_t i=0, n=vFaces.size(); i<n; ++i)
                {
                    font_entry_t *fe = vFaces.uget(i);
                    if ((fe != NULL) && (fe->face->flags == selector))
                    {
                        if (!strcmp(fe->name, id->name))
                            return fe->face;
                    }
                }

                return NULL;
            }

            face_t *FontManager::select_font_face(const Font *f)
            {
                // Walk through aliases and get the real face name
                const char *name = f->name();
                if (name == NULL)
                    return NULL;

                while (true)
                {
                    const char *aliased = vAliases.get(name);
                    if (aliased == NULL)
                        break;
                    name    = aliased;
                }

                // Now we have non-aliased name, let's look up into the cache for such font
                // (first non-synthetic, then for synthetic)
                face_t **pface;
                face_id_t id;
                size_t flags    = make_face_id_flags(f);
                id.name         = name;
                id.size         = float_to_f26p6(f->size());

//                if (id.size == 576)
//                {
//                    lltl::parray<face_id_t> vk;
//                    vFontCache.keys(&vk);
//
//                    lsp_trace("Available font in cache: ");
//                    for (size_t i=0; i<vk.size(); ++i)
//                    {
//                        face_id_t *fid = vk.uget(i);
//                        lsp_trace("  name=%s, flags=0x%x, size=%d",
//                            fid->name, int(fid->flags), int(fid->size));
//                    }
//                }

                // Try lookup the face in the face cache
                id.flags        = flags;
                if ((pface = vFontCache.wbget(&id)) != NULL)
                {
                    ++nFaceHits;
                    return *pface;
                }
                id.flags        = flags | FID_SYNTHETIC;
                if ((pface = vFontCache.wbget(&id)) != NULL)
                {
                    ++nFaceHits;
                    return *pface;
                }
                ++nFaceMisses;

                // Face was not found. Try to synthesize new face
                face_t *face    = NULL;
                switch (flags & (FID_BOLD | FID_ITALIC))
                {
                    case FID_BOLD:
                    case FID_ITALIC:
                        // Try to lookup the face with desired flags first
                        id.flags    = flags;
                        if ((face = find_face(&id)) != NULL)
                            break;
                        flags      |= FID_SYNTHETIC;

                        // Try to lookup regular face to synthesize bold or italic one
                        id.flags    = flags & (~(FID_BOLD | FID_ITALIC));
                        if ((face = find_face(&id)) != NULL)
                            break;
                        break;

                    case FID_BOLD | FID_ITALIC:
                    {
                        // Try to lookup the face with desired flags first
                        id.flags    = flags;
                        if ((face = find_face(&id)) != NULL)
                            break;
                        flags      |= FID_SYNTHETIC;

                        // Try to lookup without BOLD first
                        id.flags    = flags & (~FID_BOLD);
                        if ((face = find_face(&id)) != NULL)
                            break;

                        // Then try to lookup without ITALIC first
                        id.flags    = flags & (~FID_ITALIC);
                        if ((face = find_face(&id)) != NULL)
                            break;

                        // Then try to lookup regular one
                        id.flags    = flags & (~(FID_BOLD | FID_ITALIC));
                        if ((face = find_face(&id)) != NULL)
                            break;
                        break;
                    }

                    default:
                    case 0:
                        // Try to lookup regular face
                        id.flags    = flags;
                        if ((face = find_face(&id)) != NULL)
                            break;
                        break;
                }

                // Return if no face was found
                if (face == NULL)
                {
                    id.flags            = flags & (~FID_SYNTHETIC);
                    vFontCache.create(&id, face);
                    return NULL;
                }

                // Synthesize new face
                if ((face = clone_face(face)) == NULL)
                    return NULL;
                ++face->references;
                lsp_finally { dereference(face); };

                // Initialize synthesized face and add to the mapping
                id.flags            = flags;
                face->flags         = id.flags;
                face->h_size        = (face->ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? id.size : 0;
                face->v_size        = (face->ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ? 0 : id.size;
                face->matrix.xx     = ((face->flags & FID_BOLD) && (!(face->ft_face->style_flags & FT_STYLE_FLAG_BOLD))) ? 0x10c00 : 0x10000;
                face->matrix.xy     = ((face->flags & FID_ITALIC) && (!(face->ft_face->style_flags & FT_STYLE_FLAG_ITALIC))) ? f26p6_face_slant_shift : 0;
                face->matrix.yx     = 0;
                face->matrix.yy     = 1 * 0x10000;

                if (!vFontCache.create(&id, face))
                    return NULL;
                ++face->references;

                // Select font face
                return face;
            }

            bool FontManager::get_font_parameters(const Font *f, font_parameters_t *fp)
            {
                // Select the font face
                face_t *face    = select_font_face(f);
                if (face == NULL)
                    return false;

                if (activate_face(face) != STATUS_OK)
                    return false;

                if (fp != NULL)
                {
                    FT_Size_Metrics *metrics = & face->ft_face->size->metrics;

                    fp->Ascent  = f26p6_to_float(metrics->ascender);
                    fp->Descent = f26p6_to_float(-metrics->descender);
                    fp->Height  = f26p6_to_float(metrics->height);
                }

                return true;
            }

            bool FontManager::get_text_parameters(const Font *f, text_range_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((text == NULL) || (first > last))
                    return false;
                else if (first == last)
                {
                    tp->x_bearing       = 0;
                    tp->y_bearing       = 0;
                    tp->width           = 0;
                    tp->height          = 0;
                    tp->x_advance       = 0;
                    tp->y_advance       = 0;
                    return true;
                }

                // Select the font face
                face_t *face        = select_font_face(f);
                if (face == NULL)
                    return false;
                if (tp == NULL)
                    return true;

                if (activate_face(face) != STATUS_OK)
                    return false;

                // Estimate the text parameters
                lsp_wchar_t ch      = text->char_at(first);
                glyph_t *glyph      = get_glyph(face, ch);
                if (glyph == NULL)
                    return NULL;

                ssize_t x_bearing   = glyph->x_bearing;
                ssize_t y_bearing   = glyph->y_bearing;
                ssize_t y_max       = glyph->bitmap.height - glyph->y_bearing;
                ssize_t x           = f26p6_ceil_to_int(glyph->x_advance + glyph->lsb_delta - glyph->rsb_delta);

                for (ssize_t i = first+1; i<last; ++i)
                {
                    ch                  = text->char_at(i);
                    glyph               = get_glyph(face, ch);
                    if (glyph == NULL)
                        return NULL;

                    y_bearing           = lsp_max(y_bearing, glyph->y_bearing);
                    y_max               = lsp_max(y_max, glyph->bitmap.height - glyph->y_bearing);
                    x                  += f26p6_ceil_to_int(glyph->x_advance + glyph->lsb_delta - glyph->rsb_delta);
                }

                // Output the result
                ssize_t width       = x - x_bearing;
                ssize_t height      = y_max + y_bearing;

                tp->x_bearing       = x_bearing;
                tp->y_bearing       = -y_bearing;
                tp->width           = width;
                tp->height          = height;
                tp->x_advance       = width + x_bearing;
                tp->y_advance       = y_max + y_bearing;

                return true;
            }

            dsp::bitmap_t *FontManager::render_text(const Font *f, text_range_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((text == NULL) || (first >= last))
                    return NULL;

                // Select the font face
                face_t *face        = select_font_face(f);
                if (face == NULL)
                    return NULL;
                if (activate_face(face) != STATUS_OK)
                    return NULL;

                // Estimate the text parameters
                lsp_wchar_t ch      = text->char_at(first);
                glyph_t *glyph      = get_glyph(face, ch);
                if (glyph == NULL)
                    return NULL;

                ssize_t x_bearing   = glyph->x_bearing;
                ssize_t y_bearing   = glyph->y_bearing;
                ssize_t y_max       = glyph->bitmap.height - glyph->y_bearing;
                ssize_t x           = f26p6_ceil_to_int(glyph->x_advance + glyph->lsb_delta - glyph->rsb_delta);

                for (ssize_t i = first+1; i<last; ++i)
                {
                    ch                  = text->char_at(i);
                    glyph               = get_glyph(face, ch);
                    if (glyph == NULL)
                        return NULL;

                    y_bearing           = lsp_max(y_bearing, glyph->y_bearing);
                    y_max               = lsp_max(y_max, glyph->bitmap.height - glyph->y_bearing);
                    x                  += f26p6_ceil_to_int(glyph->x_advance + glyph->lsb_delta - glyph->rsb_delta);
                }

                // Allocate the bitmap
                ssize_t width       = x - x_bearing;
                ssize_t height      = y_max + y_bearing;

                dsp::bitmap_t *bitmap   = create_bitmap(width + (height * face->matrix.xy) / 0x10000, height);
                if (bitmap == NULL)
                    return NULL;

                // Render the contents to the bitmap
                x                   = 0;
                for (ssize_t i = first; i<last; ++i)
                {
                    ch                  = text->char_at(i);
                    glyph               = get_glyph(face, ch);
                    if (glyph == NULL)
                        return NULL;

                    ssize_t cx          = x - x_bearing + glyph->x_bearing;
                    ssize_t cy          = y_bearing - glyph->y_bearing;

                    switch (glyph->format)
                    {
                        case FMT_1_BPP:
                            dsp::bitmap_max_b1b8(bitmap, &glyph->bitmap, cx, cy);
                            break;
                        case FMT_2_BPP:
                            dsp::bitmap_max_b2b8(bitmap, &glyph->bitmap, cx, cy);
                            break;
                        case FMT_4_BPP:
                            dsp::bitmap_max_b4b8(bitmap, &glyph->bitmap, cx, cy);
                            break;
                        case FMT_8_BPP:
                        default:
                            dsp::bitmap_max_b8b8(bitmap, &glyph->bitmap, cx, cy);
                            break;
                    }

                    x                  += f26p6_ceil_to_int(glyph->x_advance + glyph->lsb_delta - glyph->rsb_delta);
                }

                if (tp != NULL)
                {
                    tp->x_bearing       = x_bearing;
                    tp->y_bearing       = -y_bearing;
                    tp->width           = width;
                    tp->height          = height;
                    tp->x_advance       = width + x_bearing;
                    tp->y_advance       = y_max + y_bearing;
                }

                return bitmap;
            }
        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



