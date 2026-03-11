/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 30 янв. 2026 г.
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


#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>
#include <lsp-plug.in/mm/Bitmap.h>

MTEST_BEGIN("ws.display", bitmap)

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;
            mm::Bitmap      sColor;
            mm::Bitmap      sChroma;

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
            }

            status_t load()
            {
                io::Path path;
                status_t res = path.fmt("%s/img/lsp-logo.xpm", pTest->resources());
                if (res <= 0)
                    return STATUS_NO_MEM;

                pTest->printf("Loading icons from %s...\n", path.as_native());

                if ((res = sColor.load(path, mm::PIXFMT_PBGRA8888, NULL)) != STATUS_OK)
                    return res;
                if ((res = sChroma.load(path, mm::PIXFMT_G8, NULL)) != STATUS_OK)
                    return res;
                if ((res = sChroma.convert(mm::PIXFMT_PBGRA8888)) != STATUS_OK)
                    return res;

                // Make background fully transparent
                for (size_t r = 0; r<sChroma.rows(); ++r)
                {
                    uint8_t *row = sChroma.row(r);
                    for (size_t c = 0; c<sChroma.columns(); ++c)
                    {
                        if ((row[0] < 0x10) && (row[1] < 0x10) && (row[2] < 0x10))
                        {
                            row[0]      = 0x00;
                            row[1]      = 0x00;
                            row[2]      = 0x00;
                            row[3]      = 0x00;
                        }
                        row += 4;
                    }
                }

                return STATUS_OK;
            }

        public:

            virtual status_t handle_event(const ws::event_t *ev) override
            {
                switch (ev->nType)
                {
                    case ws::UIE_REDRAW:
                    {
                        Color c(1.0f, 1.0f, 1.0f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(c);
                        lsp_finally{ s->end(); };

                        float x = s->width() * 0.25f;
                        float y = s->height() * 0.5f;

                        s->draw_raw(
                            sColor.data(),
                            sColor.width(), sColor.height(), sColor.stride(),
                            x - sColor.width() * 0.5f, y - sColor.height() * 0.5f,
                            1.0f, 1.0f, 0.0f);

                        x = s->width() * 0.75f;

                        s->draw_raw(
                            sChroma.data(),
                            sChroma.width(), sChroma.height(), sChroma.stride(),
                            x - sChroma.width() * 0.5f, y - sChroma.height() * 0.5f,
                            1.0f, 1.0f, 0.0f);

                        return STATUS_OK;
                    }

                    case ws::UIE_CLOSE:
                    {
                        pWnd->hide();
                        pWnd->display()->quit_main();
                        break;
                    }

                    default:
                        return IEventHandler::handle_event(ev);
                }

                return STATUS_OK;
            }
    };

    MTEST_MAIN
    {
        ws::IDisplay *dpy = ws::create_display(0, NULL);
        MTEST_ASSERT(dpy != NULL);
        lsp_finally { ws::free_display(dpy); };

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test bitmap drawing") == STATUS_OK);
        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE | ws::WA_RESIZE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(320, 200, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);
        MTEST_ASSERT(h.load() == STATUS_OK);

        MTEST_ASSERT(wnd->show() == STATUS_OK);

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END






