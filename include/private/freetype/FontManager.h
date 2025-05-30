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

#ifndef PRIVATE_FREETYPE_FONTMANAGER_H_
#define PRIVATE_FREETYPE_FONTMANAGER_H_

#ifdef USE_LIBFREETYPE

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/lltl/phashset.h>

#include <private/freetype/types.h>
#include <private/freetype/bitmap.h>
#include <private/freetype/face.h>
#include <private/freetype/face_id.h>
#include <private/freetype/glyph.h>
#include <private/freetype/GlyphCache.h>
#include <private/freetype/LRUCache.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Custom font manager
             */
            class LSP_HIDDEN_MODIFIER FontManager
            {
                private:
                    typedef struct font_entry_t
                    {
                        char           *name;       // The name of font entry
                        face_t         *face;       // The pointer to the face
                    } font_entry_t;

                private:
                    FT_Library                          hLibrary;
                    lltl::darray<font_entry_t>          vFaces;
                    lltl::pphash<face_id_t, face_t>     vFontCache;
                    lltl::pphash<char, char>            vAliases;
                    LRUCache                            sLRU;
                    size_t                              nCacheSize;
                    size_t                              nMinCacheSize;
                    size_t                              nMaxCacheSize;
                    size_t                              nFaceHits;
                    size_t                              nFaceMisses;
                    size_t                              nGlyphHits;
                    size_t                              nGlyphMisses;
                    size_t                              nGlyphRemoval;

                protected:
                    glyph_t                *get_glyph(face_t *face, lsp_wchar_t ch);
                    void                    invalidate_faces(const char *name);
                    void                    invalidate_face(face_t *face);
                    static bool             add_font_face(lltl::darray<font_entry_t> *entries, const char *name, face_t *face);
                    static void             dereference(face_t *face);
                    face_t                 *select_font_face(const Font *f);
                    face_t                 *find_face(const face_id_t *id);
                    face_t                 *lookup_face(const face_id_t *id);

                public:
                    FontManager();
                    ~FontManager();

                    status_t                init();
                    void                    destroy();

                public:
                    status_t                add(const char *name, const char *path);
                    status_t                add(const char *name, const io::Path *path);
                    status_t                add(const char *name, const LSPString *path);
                    status_t                add(const char *name, io::IInStream *is);
                    status_t                add_alias(const char *name, const char *alias);
                    status_t                remove(const char *name);
                    status_t                clear();

                    /**
                     * Get font parameters
                     * @param f font descriptor
                     * @param fp pointer to store font parameters
                     * @return true if corresponding font has been found
                     */
                    bool                    get_font_parameters(const Font *f, font_parameters_t *fp);

                    /**
                     * Get text paramerers
                     * @param f font descriptor
                     * @param tp pointer to store text parameters
                     * @param text the text to estimate parameters
                     * @param first first character of substring in the string
                     * @param last last character of substring in the string
                     * @return true if corresponding font has been found and text has been processed
                     */
                    bool                    get_text_parameters(const Font *f, text_range_t *tp, const LSPString *text, ssize_t first, ssize_t last);

                    /**
                     * Render text to bitmap
                     * @param f font descriptor
                     * @param text text to render
                     * @param first first character of substring in the string
                     * @param last last character of substring in the string
                     * @return pointer to bitmap or NULL if corresponding font has not been found
                     */
                    dsp::bitmap_t          *render_text(const Font *f, text_range_t *tp, const LSPString *text, ssize_t first, ssize_t last);

                public: // Cache control and statistics
                    /**
                     * Perform garbage collection
                     */
                    void                    gc();

                    void                    set_cache_limits(size_t min, size_t max);
                    size_t                  set_min_cache_size(size_t min);
                    size_t                  set_max_cache_size(size_t max);

                    inline size_t           min_cache_size() const  { return nMinCacheSize; }
                    inline size_t           max_cache_size() const  { return nMaxCacheSize; }
                    inline size_t           used_cache_size() const { return nCacheSize;    }

                    inline size_t           face_hits() const       { return nFaceHits;     }
                    inline size_t           face_misses() const     { return nFaceMisses;   }
                    inline size_t           glyph_hits() const      { return nGlyphHits;    }
                    inline size_t           glyph_misses() const    { return nGlyphMisses;  }
                    inline size_t           glyph_removal() const   { return nGlyphRemoval; }
                    void                    clear_cache_stats();
            };

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



#endif /* PRIVATE_FREETYPE_FONTMANAGER_H_ */
