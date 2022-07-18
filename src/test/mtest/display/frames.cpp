/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 7 июл. 2022 г.
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

#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>
#include <lsp-plug.in/test-fw/mtest.h>

static const uint32_t colors[9] =
{
    0xff0000,
    0x00ff00,
    0x0000ff,
    0xffff00,
    0xff00ff,
    0x00ffff,
    0xffcc00,
    0xcc00ff,
    0x00ccff
};

MTEST_BEGIN("ws.display", frames)

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
                        Color c(0.0f, 0.0f, 0.0f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(c);

                        ws::rectangle_t r1, r2;
                        r1.nWidth       = 104;
                        r1.nHeight      = 64;
                        r2.nWidth       = r1.nWidth  / 2;
                        r2.nHeight      = r1.nHeight / 2;

                        // Method 1: fill frame using coordinates
                        for (size_t y=0; y<3; ++y)
                            for (size_t x=0; x<3; ++x)
                            {
                                c.set_rgb24(colors[y*3 + x]);
                                r1.nLeft = 4 + x * r1.nWidth;
                                r1.nTop  = 4 + y * r1.nHeight;
                                r2.nLeft = r1.nLeft + (x * r2.nWidth)  / 2;
                                r2.nTop  = r1.nTop  + (y * r2.nHeight) / 2;

                                s->fill_frame(
                                    c, SURFMASK_NONE, 0.0f,
                                    r1.nLeft, r1.nTop, r1.nWidth, r1.nHeight,
                                    r2.nLeft, r2.nTop, r2.nWidth, r2.nHeight);
                            }

                        // Method 2: fill frame using rectangles
                        for (size_t y=0; y<3; ++y)
                            for (size_t x=0; x<3; ++x)
                            {
                                c.set_rgb24(colors[y*3 + x]);
                                r1.nLeft = 324 + x * r1.nWidth;
                                r1.nTop  = 4 + y * r1.nHeight;
                                r2.nLeft = r1.nLeft + (x * r2.nWidth)  / 2;
                                r2.nTop  = r1.nTop  + (y * r2.nHeight) / 2;

                                s->fill_frame(c, SURFMASK_NONE, 0.0f, &r1, &r2);
                            }

                        // Method 3: fill round frame using coordinates
                        size_t mask = 0;
                        for (size_t y=0; y<3; ++y)
                            for (size_t x=0; x<3; ++x)
                            {
                                c.set_rgb24(colors[y*3 + x]);
                                r1.nLeft = 4 + x * r1.nWidth;
                                r1.nTop  = 204 + y * r1.nHeight;
                                r2.nLeft = r1.nLeft + (x * r2.nWidth)  / 2;
                                r2.nTop  = r1.nTop  + (y * r2.nHeight) / 2;

                                s->fill_frame(
                                    c, mask, 12,
                                    r1.nLeft, r1.nTop, r1.nWidth, r1.nHeight,
                                    r2.nLeft, r2.nTop, r2.nWidth, r2.nHeight);

                                ++mask;
                            }

                        // Method 4: fill round frame using coordinates
                        for (size_t y=0; y<3; ++y)
                            for (size_t x=0; x<3; ++x)
                            {
                                c.set_rgb24(colors[y*3 + x]);
                                r1.nLeft = 324 + x * r1.nWidth;
                                r1.nTop  = 204 + y * r1.nHeight;
                                r2.nLeft = r1.nLeft + (x * r2.nWidth)  / 2;
                                r2.nTop  = r1.nTop  + (y * r2.nHeight) / 2;

                                s->fill_frame(c, mask, 12, &r1, &r2);

                                ++mask;
                            }

                        s->end();

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
        lsp_finally { ws::lsp_ws_free_display(dpy); };

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test frames") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



