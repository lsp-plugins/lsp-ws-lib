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
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/test-fw/utest.h>

#include <private/freetype/FontManager.h>

using namespace lsp::ws;

namespace
{
    using namespace lsp;

    int write_bitmap(const dsp::bitmap_t *b, const io::Path *file)
    {
        FILE *fd = fopen(file->as_native(), "w");
        if (fd == NULL)
            return -1;

        lsp_finally { fclose(fd); };

        fprintf(fd, "/* XPM */\n");
        fprintf(fd, "static char * test_xpm[] = {\n");

        const uint8_t *buf = b->data;
        static const char *dict = "0123456789abcdef";
        constexpr size_t N = 256;

        fprintf(fd, "\"%d %d %d 2\",\n", int(b->width), int(b->height), int(N));
        for (size_t i=0; i<N; ++i)
        {
            int alpha = i ^ 0xff;
            fprintf(fd, "\"%c%c\tc #%02x%02x%02x\",\n", dict[(i >> 4) & 0x0f], dict[i & 0x0f], alpha, alpha, alpha);
        }

        for (ssize_t y=0; y<b->height; ++y, buf += b->stride)
        {
            fputc('"', fd);

            for (ssize_t x=0; x<b->width; ++x)
            {
                uint8_t alpha = buf[x];
                fputc(dict[(alpha >> 4) & 0x0f], fd);
                fputc(dict[alpha & 0x0f], fd);
            }

            fputc('"', fd);
            if ((y + 1) < b->height)
                fputc(',', fd);
            fputc('\n', fd);
        }

        fprintf(fd, "}\n");

        return 0;
    }
} /* namespace */;

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
        UTEST_ASSERT(manager.add("test-1", &path1) == STATUS_OK);

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
        UTEST_ASSERT(manager.remove("test-1") == STATUS_NOT_FOUND);

        // Remove aliases
        UTEST_ASSERT(manager.remove("alias-test-1") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-2") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-3") == STATUS_OK);
        UTEST_ASSERT(manager.remove("alias-test-4") == STATUS_NOT_FOUND);
    }

    void test_render_text()
    {
        // Load font
        ft::FontManager manager;
        io::Path path;

        printf("Testing text rendering\n");

        // Initialize manager
        UTEST_ASSERT(manager.init() == STATUS_OK);
        lsp_finally { manager.destroy(); };
        UTEST_ASSERT(path.fmt("%s/font/NotoSansDisplay-Regular.ttf", resources()) > 0);
        UTEST_ASSERT(manager.add("noto-sans", &path) == STATUS_OK);

        // Try to render text
        ft::text_range_t tp;
        ws::Font f("noto-sans", 12.0f);
        f.set_bold(true);
        LSPString text;
        UTEST_ASSERT(text.set_ascii("Hello World! This is tiny test text output."));

        dsp::bitmap_t *bitmap = manager.render_text(&f, &tp, &text, 0, text.length());
        UTEST_ASSERT(bitmap != NULL);
        lsp_finally { ft::free_bitmap(bitmap); };

        // Save rendered text
        UTEST_ASSERT(path.fmt("%s/%s-test-hello-world.xpm", tempdir(), name()) > 0);
        UTEST_ASSERT(write_bitmap(bitmap, &path) == 0);

        printf("Output file:        %s\n", path.as_native());
        printf("Image Size:         %d x %d\n", int(bitmap->width), int(bitmap->height));
        printf("Stride:             %d\n", int(bitmap->stride));
        printf("Bearing:            %d, %d\n", int(tp.x_bearing), int(tp.y_bearing));
        printf("Size:               %d x %d\n", int(tp.width), int(tp.height));
        printf("Advance:            %d, %d\n", int(tp.x_advance), int(tp.y_advance));
        printf("Used cache size:    %ld bytes\n", long(manager.used_cache_size()));
        printf("Cache hit/miss/rm:  %ld/%ld/%ld\n", long(manager.cache_hits()), long(manager.cache_misses()), long(manager.cache_removal()));

        // Remove the font
        UTEST_ASSERT(manager.remove("noto-sans") == STATUS_OK);
    }

    void test_fail_render_text()
    {
        // Load font
        ft::FontManager manager;
        io::Path path;

        printf("Testing failed text rendering\n");

        // Initialize manager
        UTEST_ASSERT(manager.init() == STATUS_OK);
        lsp_finally { manager.destroy(); };

        // Try to render text
        ft::text_range_t tp;
        ws::Font f("noto-sans", 12.0f);
        f.set_italic(true);
        LSPString text;
        UTEST_ASSERT(text.set_ascii("Another one text for test rendering"));

        // Test first (long) search of the font face
        dsp::bitmap_t *bitmap = manager.render_text(&f, &tp, &text, 0, text.length());
        UTEST_ASSERT(bitmap == NULL);

        // Test second (quick) search of the font face
        bitmap = manager.render_text(&f, &tp, &text, 0, text.length());
        UTEST_ASSERT(bitmap == NULL);

        // Load font and invalidate cache for the 'noto-sans' font
        UTEST_ASSERT(path.fmt("%s/font/NotoSansDisplay-Regular.ttf", resources()) > 0);
        UTEST_ASSERT(manager.add("noto-sans", &path) == STATUS_OK);

        // Now the rendering should be OK
        bitmap = manager.render_text(&f, &tp, &text, 0, text.length());
        UTEST_ASSERT(bitmap != NULL);
        lsp_finally { ft::free_bitmap(bitmap); };

        // Save rendered text
        UTEST_ASSERT(path.fmt("%s/%s-test-italic.xpm", tempdir(), name()) > 0);
        UTEST_ASSERT(write_bitmap(bitmap, &path) == 0);

        printf("Output file:        %s\n", path.as_native());
        printf("Image Size:         %d x %d\n", int(bitmap->width), int(bitmap->height));
        printf("Stride:             %d\n", int(bitmap->stride));
        printf("Bearing:            %d, %d\n", int(tp.x_bearing), int(tp.y_bearing));
        printf("Size:               %d x %d\n", int(tp.width), int(tp.height));
        printf("Advance:            %d, %d\n", int(tp.x_advance), int(tp.y_advance));
        printf("Used cache size:    %ld bytes\n", long(manager.used_cache_size()));
        printf("Cache hit/miss/rm:  %ld/%ld/%ld\n", long(manager.cache_hits()), long(manager.cache_misses()), long(manager.cache_removal()));

        // Remove the font
        UTEST_ASSERT(manager.remove("noto-sans") == STATUS_OK);
    }

    UTEST_MAIN
    {
//        test_load_font();
//        test_render_text();
        test_fail_render_text();
    }

UTEST_END;

#endif /* USE_LIBFREETYPE */




