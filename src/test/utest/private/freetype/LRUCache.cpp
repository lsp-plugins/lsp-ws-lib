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

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/test-fw/utest.h>

#include <private/freetype/LRUCache.h>

UTEST_BEGIN("ws.freetype", lrucache)

    bool validate_lru_consistency(lsp::ws::ft::LRUCache *lru)
    {
        lltl::parray<lsp::ws::ft::glyph_t> processed;
        lsp::ws::ft::glyph_t *glyph;;

        // Check the 'next' chain
        for (glyph = lru->pHead; glyph != NULL; glyph = glyph->next)
        {
            if (processed.contains(glyph))
                return false;
            processed.push(glyph);
        }

        // Check consistency
        if (processed.size() > 0)
        {
            if(processed.last() != lru->pTail)
                return false;
        }
        else if ((lru->pHead != NULL) || (lru->pTail != NULL))
            return false;

        // Check the 'prev' chain
        for (glyph = lru->pTail; glyph != NULL; glyph = glyph->prev)
        {
            lsp::ws::ft::glyph_t *curr = processed.pop();
            if (curr != glyph)
                return false;
        }
        return (processed.is_empty());
    }

    bool check_lru_state(lsp::ws::ft::LRUCache *lru, const char *state)
    {
        if (!validate_lru_consistency(lru))
        {
            printf("Failed LRU consistency check\n");
            return false;
        }

        LSPString s;
        for (const lsp::ws::ft::glyph_t *glyph = lru->pHead; glyph != NULL; glyph = glyph->next)
        {
            if (!s.append(glyph->codepoint))
            {
                printf("Out of memory\n");
                return false;
            }
        }

        if (s.equals_ascii(state))
            return true;

        printf("Expected LRU state: '%s', actual LRU state: '%s'\n", state, s.get_utf8());
        return false;
    }

    void test_add_first()
    {
        printf("Testing add_first...\n");

        lsp::ws::ft::glyph_t glyphs[6];
        for (size_t i=0; i<6; ++i)
            glyphs[i].codepoint = 'A' + i;

        lsp::ws::ft::LRUCache lru;
        UTEST_ASSERT(check_lru_state(&lru, ""));
        lru.add_first(&glyphs[0]);
        UTEST_ASSERT(check_lru_state(&lru, "A"));
        lru.add_first(&glyphs[1]);
        UTEST_ASSERT(check_lru_state(&lru, "BA"));
        lru.add_first(&glyphs[2]);
        UTEST_ASSERT(check_lru_state(&lru, "CBA"));
        lru.add_first(&glyphs[3]);
        UTEST_ASSERT(check_lru_state(&lru, "DCBA"));
        lru.add_first(&glyphs[4]);
        UTEST_ASSERT(check_lru_state(&lru, "EDCBA"));
        lru.add_first(&glyphs[5]);
        UTEST_ASSERT(check_lru_state(&lru, "FEDCBA"));

        lru.clear();
        UTEST_ASSERT(check_lru_state(&lru, ""));
    }

    void test_remove_last()
    {
        printf("Testing remove_last...\n");

        lsp::ws::ft::glyph_t glyphs[6];
        for (size_t i=0; i<6; ++i)
            glyphs[i].codepoint = 'A' + i;

        lsp::ws::ft::LRUCache lru;
        UTEST_ASSERT(check_lru_state(&lru, ""));
        for (size_t i=0; i<6; ++i)
            lru.add_first(&glyphs[i]);

        UTEST_ASSERT(check_lru_state(&lru, "FEDCBA"));

        UTEST_ASSERT(lru.remove_last() == &glyphs[0]);
        UTEST_ASSERT(check_lru_state(&lru, "FEDCB"));
        UTEST_ASSERT(lru.remove_last() == &glyphs[1]);
        UTEST_ASSERT(check_lru_state(&lru, "FEDC"));
        UTEST_ASSERT(lru.remove_last() == &glyphs[2]);
        UTEST_ASSERT(check_lru_state(&lru, "FED"));
        UTEST_ASSERT(lru.remove_last() == &glyphs[3]);
        UTEST_ASSERT(check_lru_state(&lru, "FE"));
        UTEST_ASSERT(lru.remove_last() == &glyphs[4]);
        UTEST_ASSERT(check_lru_state(&lru, "F"));
        UTEST_ASSERT(lru.remove_last() == &glyphs[5]);
        UTEST_ASSERT(check_lru_state(&lru, ""));
    }

    void test_remove()
    {
        printf("Testing remove...\n");

        lsp::ws::ft::glyph_t glyphs[6];
        for (size_t i=0; i<6; ++i)
            glyphs[i].codepoint = 'A' + i;

        lsp::ws::ft::LRUCache lru;
        UTEST_ASSERT(check_lru_state(&lru, ""));
        for (size_t i=0; i<6; ++i)
            lru.add_first(&glyphs[i]);

        UTEST_ASSERT(check_lru_state(&lru, "FEDCBA"));

        lru.remove(&glyphs[2]);
        UTEST_ASSERT(check_lru_state(&lru, "FEDBA"));
        lru.remove(&glyphs[3]);
        UTEST_ASSERT(check_lru_state(&lru, "FEBA"));
        lru.remove(&glyphs[0]);
        UTEST_ASSERT(check_lru_state(&lru, "FEB"));
        lru.remove(&glyphs[5]);
        UTEST_ASSERT(check_lru_state(&lru, "EB"));
        lru.remove(&glyphs[4]);
        UTEST_ASSERT(check_lru_state(&lru, "B"));
        lru.remove(&glyphs[1]);
        UTEST_ASSERT(check_lru_state(&lru, ""));
    }

    void test_touch()
    {
        printf("Testing touch...\n");

        lsp::ws::ft::glyph_t glyphs[6];
        for (size_t i=0; i<6; ++i)
            glyphs[i].codepoint = 'A' + i;

        lsp::ws::ft::LRUCache lru;
        UTEST_ASSERT(check_lru_state(&lru, ""));
        for (size_t i=0; i<6; ++i)
            lru.add_first(&glyphs[i]);

        UTEST_ASSERT(check_lru_state(&lru, "FEDCBA"));

        UTEST_ASSERT(lru.touch(&glyphs[0]));
        UTEST_ASSERT(check_lru_state(&lru, "AFEDCB"));
        UTEST_ASSERT(lru.touch(&glyphs[0]));
        UTEST_ASSERT(check_lru_state(&lru, "AFEDCB"));

        UTEST_ASSERT(lru.touch(&glyphs[2]));
        UTEST_ASSERT(check_lru_state(&lru, "CAFEDB"));
        UTEST_ASSERT(lru.touch(&glyphs[1]));
        UTEST_ASSERT(check_lru_state(&lru, "BCAFED"));
        UTEST_ASSERT(lru.touch(&glyphs[3]));
        UTEST_ASSERT(check_lru_state(&lru, "DBCAFE"));
    }

    UTEST_MAIN
    {
        test_add_first();
        test_remove_last();
        test_remove();
        test_touch();
    }

UTEST_END;

#endif /* USE_LIBFREETYPE */


