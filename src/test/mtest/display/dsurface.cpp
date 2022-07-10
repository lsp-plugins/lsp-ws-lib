/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 9 июл. 2022 г.
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

MTEST_BEGIN("ws.display", dsurface)

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
            }

            virtual status_t handle_event(const ws::event_t *ev)
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
                        lsp_finally( s->end(); );

                        uint32_t *buf = static_cast<uint32_t *>(malloc( 320 * 200 * sizeof(uint32_t)));
                        lsp_finally( free(buf); );

                        for (size_t y=0; y<200; ++y)
                        {
                            uint32_t *row = &buf[y * 320];
                            uint32_t alpha= uint8_t((0xff * y) / (200 - 1)) << 24;
                            for (size_t x=0; x<320; ++x)
                                row[x]  = CPU_TO_LE(uint32_t((x * y) | alpha));
                        }

                        s->draw_raw(
                            buf, 320, 200, 320 * sizeof(uint32_t),
                            0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                        s->draw_raw(
                            buf, 320, 200, 320 * sizeof(uint32_t),
                            320.0f, 200.0f, 1.0f, 1.0f, 0.0f);

                        for (size_t y=0; y<200; ++y)
                        {
                            uint32_t *row = &buf[y * 320];
                            for (size_t x=0; x<320; ++x)
                                row[x]  = CPU_TO_LE(LE_TO_CPU(row[x]) ^ 0x00ffffff);
                        }

                        s->draw_raw(
                            buf, 320, 200, 320 * sizeof(uint32_t),
                            320.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                        s->draw_raw(
                            buf, 320, 200, 320 * sizeof(uint32_t),
                            0.0f, 200.0f, 1.0f, 1.0f, 0.0f);

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
        ws::IDisplay *dpy = ws::lsp_ws_create_display(0, NULL);
        MTEST_ASSERT(dpy != NULL);
        lsp_finally( ws::lsp_ws_free_display(dpy); );

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally(
            wnd->destroy();
            delete wnd;
        );

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test direct surface") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



