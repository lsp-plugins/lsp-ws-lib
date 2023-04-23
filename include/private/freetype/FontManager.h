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
#include <private/freetype/face.h>
#include <private/freetype/glyph.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            /**
             * Custom font manager
             */
            class FontManager
            {
                private:
                    typedef struct font_t
                    {
                        size_t          references; // Number of references
                        uint8_t        *data;       // The actual data for the font stored in memory
                        FT_Face         ft_face;    // The font face associated with the data
                        const char     *family;     // Font family
                        const char     *style;      // Font style
                        const char     *name;       // The font name in the system
                    } font_t;

                private:
                    lltl::parray<font_t>                vCustomFonts;
                    lltl::pphash<Font, face_t>          vFontMapping;
                    size_t                              nCacheSize;
                    size_t                              nMinCacheSize;
                    size_t                              nMaxCacheSize;
                    glyph_t                            *pHead;      // Head in the LRU cache
                    glyph_t                            *pTail;      // Tail in the LRU cache

                protected:
                    glyph_t            *get_glyph(face_t *face, lsp_wchar_t ch);
                    inline void         lru_remove_glyph(glyph_t *glyph);
                    inline glyph_t     *lru_remove_last();
                    inline glyph_t     *lru_add_first(glyph_t *glyph);
                    inline glyph_t     *lru_touch(glyph_t *glyph);

                public:
                    FontManager();
                    ~FontManager();

                public:
                    status_t            add_font(const char *name, io::IInStream *is);
                    status_t            add_font_alias(const char *name, const char *alias);
                    status_t            remove_font(const char *name);
                    void                clear();
                    void                gc();

                    void                set_cache_limits(size_t min, size_t max);
                    size_t              set_min_cache_size(size_t min);
                    size_t              set_max_cache_size(size_t max);

                    size_t              min_cache_size() const;
                    size_t              max_cache_size() const;
                    size_t              used_cache_size() const;

                    /**
                     * Get font parameters
                     * @param f font descriptor
                     * @param fp pointer to store font parameters
                     * @return true if corresponding font has been found
                     */
                    bool                get_font_parameters(const Font *f, font_parameters_t *fp);

                    /**
                     * Get text paramerers
                     * @param f font descriptor
                     * @param tp pointer to store text parameters
                     * @param text the text to estimate parameters
                     * @param first first character of substring in the string
                     * @param last last character of substring in the string
                     * @return true if corresponding font has been found and text has been processed
                     */
                    bool                get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last);

                    /**
                     * Render text to bitmap
                     * @param text text to render
                     * @param first first character of substring in the string
                     * @param last last character of substring in the string
                     * @return pointer to bitmap or NULL if corresponding font has not been found
                     */
                    dsp::bitmap_t      *render_text(const LSPString *text, size_t first, size_t last);

            };

        } /* namespace ft */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBFREETYPE */



#endif /* PRIVATE_FREETYPE_FONTMANAGER_H_ */
