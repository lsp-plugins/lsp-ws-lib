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

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/test-fw/utest.h>

#include <private/freetype/GlyphCache.h>

using namespace lsp::ws;

UTEST_BEGIN("ws.freetype", glyphcache)

    void test_add_get()
    {
        printf("Testing adding and getting operations\n");

        const char *glyphs="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}/?.";
        ft::GlyphCache cache;

        size_t count = strlen(glyphs);
        ft::glyph_t *vglyphs = new ft::glyph_t[count];
        UTEST_ASSERT(vglyphs != NULL);
        lsp_finally { delete [] vglyphs; };

        // Put all glyphs to the cache
        for (size_t i=0; i<count; ++i)
        {
            ft::glyph_t *glyph  = &vglyphs[i];
            glyph->codepoint    = glyphs[i];
            UTEST_ASSERT_MSG(cache.put(glyph), "Failed to put glyph '%c'", glyphs[i]);
        }

        UTEST_ASSERT(cache.size() == count);

        // Check that all glyphs are present the cache
        for (size_t i=0; i<count; ++i)
        {
            ft::glyph_t *glyph  = &vglyphs[i];
            UTEST_ASSERT(cache.get(glyph->codepoint) == glyph);
        }
    }

    void test_add_remove()
    {
        printf("Testing adding and removing operations\n");

        const char *glyphs="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}/?.";
        ft::GlyphCache cache;

        size_t count = strlen(glyphs);
        ft::glyph_t *vglyphs = new ft::glyph_t[count];
        UTEST_ASSERT(vglyphs != NULL);
        lsp_finally { delete [] vglyphs; };

        // Put all glyphs to the cache
        for (size_t i=0; i<count; ++i)
        {
            ft::glyph_t *glyph  = &vglyphs[i];
            glyph->codepoint    = glyphs[i];
            UTEST_ASSERT(cache.put(glyph));
        }

        UTEST_ASSERT(cache.size() == count);

        // Remove all glyphs from the cache
        for (size_t i=0; i<count; ++i)
        {
            ft::glyph_t *glyph  = &vglyphs[i];
            UTEST_ASSERT(cache.remove(glyph));
        }

        UTEST_ASSERT(cache.size() == 0);
    }

    void test_clear()
    {
        printf("Testing clear operation\n");

        const char *glyphs="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}/?.";
        LSPString list;
        ft::GlyphCache cache;

        size_t count = strlen(glyphs);
        ft::glyph_t *vglyphs = new ft::glyph_t[count];
        UTEST_ASSERT(vglyphs != NULL);
        lsp_finally { delete [] vglyphs; };

        // Put all glyphs to the cache
        for (size_t i=0; i<count; ++i)
        {
            ft::glyph_t *glyph  = &vglyphs[i];
            glyph->codepoint    = glyphs[i];
            UTEST_ASSERT(cache.put(glyph));
        }

        UTEST_ASSERT(cache.size() == count);

        // Remove all glyphs from the cache
        ft::glyph_t *root   = cache.clear();
        UTEST_ASSERT(list.set_ascii(glyphs));
        UTEST_ASSERT(cache.size() == 0);

        for ( ; root != NULL; root = root->cache_next)
        {
            ssize_t idx = list.index_of(root->codepoint);
            UTEST_ASSERT(idx >= 0);
            UTEST_ASSERT(list.remove(idx, idx + 1));
        }

        UTEST_ASSERT(list.is_empty());
    }

    void test_invalid_operations()
    {
        printf("Testing invalid operations\n");

        ft::GlyphCache cache;
        ft::glyph_t vglyphs[2];
        vglyphs[0].codepoint    = 'A';
        vglyphs[1].codepoint    = 'B';

        UTEST_ASSERT(cache.put(&vglyphs[0]));
        UTEST_ASSERT(cache.put(&vglyphs[1]));
        UTEST_ASSERT(!cache.put(&vglyphs[0]));
        UTEST_ASSERT(!cache.put(&vglyphs[1]));

        UTEST_ASSERT(cache.get('A') == &vglyphs[0]);
        UTEST_ASSERT(cache.get('B') == &vglyphs[1]);
        UTEST_ASSERT(cache.get('C') == NULL);

        UTEST_ASSERT(cache.remove(&vglyphs[0]));
        UTEST_ASSERT(cache.remove(&vglyphs[1]));
        UTEST_ASSERT(!cache.remove(&vglyphs[0]));
        UTEST_ASSERT(!cache.remove(&vglyphs[1]));
    }

    UTEST_MAIN
    {
        test_add_get();
        test_add_remove();
        test_clear();
        test_invalid_operations();
    }

UTEST_END;

#endif /* USE_LIBFREETYPE */


