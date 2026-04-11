/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 11 апр. 2026 г.
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

MTEST_BEGIN("ws.display", multimain)

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;
            Color           sColor;

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
            }

            inline Color   &color()
            {
                return sColor;
            }

            virtual status_t handle_event(const ws::event_t *ev)
            {
                switch (ev->nType)
                {
                    case ws::UIE_MOUSE_CLICK:
                        pTest->printf("CLICK\n");
                        break;

                    case ws::UIE_MOUSE_DBL_CLICK:
                        pTest->printf("DBL_CLICK\n");
                        break;

                    case ws::UIE_MOUSE_TRI_CLICK:
                        pTest->printf("TRI_CLICK\n");
                        break;

                    case ws::UIE_REDRAW:
                    {
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(sColor);
                        s->end();

                        return STATUS_OK;
                    }

                    case ws::UIE_MOUSE_MOVE:
                    {
                        size_t screen;
                        ssize_t left, top;

                        if (pWnd->display()->get_pointer_location(&screen, &left, &top) == STATUS_OK)
                            pTest->printf("Pointer location: local=(%d, %d), screen=(%d, %d, %d)\n",
                                int(ev->nLeft), int(ev->nTop), int(left), int(top), int(screen));
                        return STATUS_OK;
                    };

                    case ws::UIE_CLOSE:
                    {
                        pWnd->hide();
                        pWnd->display()->quit_main();
                        break;
                    }

                    case ws::UIE_STATE:
                    {
                        pTest->printf("Current window state is: %d\n", int(ev->nCode));
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

        // Create window and do the stuff
        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };
        MTEST_ASSERT(wnd->init() == STATUS_OK);

        LSPString dst;
        MTEST_ASSERT(wnd->set_caption("Test multiple main") == STATUS_OK);
        MTEST_ASSERT(wnd->set_border_style(ws::BS_SIZEABLE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_RESIZE | ws::WA_CLOSE | ws::WA_MAXIMIZE | ws::WA_MINIMIZE) == STATUS_OK);

        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(160, 100, 640, 400) == STATUS_OK);

        size_t screen = wnd->screen();
        ssize_t sw, sh;
        ws::rectangle_t wr;
        MTEST_ASSERT(dpy->screen_size(screen, &sw, &sh) == STATUS_OK);
        MTEST_ASSERT(wnd->get_absolute_geometry(&wr) == STATUS_OK);
        wnd->move((sw - wr.nWidth) / 2, (sh - wr.nHeight) / 2);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        // Do the first launch
        h.color().set_rgb(0.0f, 0.75f, 0.0f);
        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
        MTEST_ASSERT(dpy->process_pending_events() == STATUS_OK);

        // Do the second launch
        h.color().set_rgb(1.0f, 1.0f, 0.0f);
        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



