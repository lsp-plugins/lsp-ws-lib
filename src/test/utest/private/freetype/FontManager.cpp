/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 28 апр. 2023 г.
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
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/test-fw/utest.h>

#include <private/freetype/FontManager.h>

using namespace lsp::ws;

UTEST_BEGIN("ws.freetype", fontmanager)

    void test_load_font()
    {
        printf("Testing loading of the font\n");
        ft::FontManager manager;
        io::Path path1, path2;

        // Initialize manager
        UTEST_ASSERT(manager.init() == STATUS_OK);
        lsp_finally { manager.destroy(); };

        // Add first font
        UTEST_ASSERT(path1.fmt("%s/font/example.ttf", resources()) > 0);
        UTEST_ASSERT(manager.add_font("test-1", &path1) == STATUS_OK);

        // Add second font
        io::InFileStream ifs;
        UTEST_ASSERT(path2.fmt("%s/font/lsp-icons.ttf", resources()) > 0);
        UTEST_ASSERT(ifs.open(&path2) == STATUS_OK);
        UTEST_ASSERT(manager.add("test-2", &ifs) == STATUS_OK);

        // Create aliases
        UTEST_ASSERT(manager.add_alias("alias-test-1", "test-1") == STATUS_OK);
        UTEST_ASSERT(manager.add_alias("alias-test-2", "test-2") == STATUS_OK);
        UTEST_ASSERT(manager.add_alias("alias-test-3", "test-3") == STATUS_OK);
        UTEST_ASSERT(manager.add_alias("alias-test-1", "test-3") == STATUS_ALREADY_EXISTS);

        // Remove fonts
        UTEST_ASSERT(manager.remove("test-2") == STATUS_OK);
        UTEST_ASSERT(manager.remove("test-1") == STATUS_OK);
        UTEST_ASSERT(manager.remove("test-2") == STATUS_NOT_FOUND);

        // Remove aliases
        UTEST_ASSERT(manager.remove("alias-test-1") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-2") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-3") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-4") == STATUS_NOT_FOUND);
    }

    UTEST_MAIN
    {
        test_load_font();
    }

UTEST_END;

#endif /* USE_LIBFREETYPE */




