/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 17 июл. 2022 г.
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

#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>

MTEST_BEGIN("ws.display", events)

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

            status_t log_mouse_event(const char *event, const ws::event_t *ev, const char *extra = NULL)
            {
                size_t screen;
                ssize_t left, top;

                if (pWnd->display()->get_pointer_location(&screen, &left, &top) == STATUS_OK)
                    pTest->printf("%s: local=(%d, %d), screen=(%d, %d, %d)%s\n",
                        event, int(ev->nLeft), int(ev->nTop), int(left), int(top), int(screen),
                        (extra) ? extra : "");

                return STATUS_OK;
            }

            virtual status_t handle_event(const ws::event_t *ev)
            {
                char buf[80];

                switch (ev->nType)
                {
                    // Mouse events
                    case ws::UIE_MOUSE_DOWN:
                        snprintf(buf, sizeof(buf), " button=%d (0x%d), state=0x%x", int(ev->nCode), int(ev->nCode), int(ev->nState));
                        return log_mouse_event("MOUSE_DOWN", ev, buf);
                    case ws::UIE_MOUSE_UP:
                        snprintf(buf, sizeof(buf), " button=%d (0x%d), state=0x%x", int(ev->nCode), int(ev->nCode), int(ev->nState));
                        return log_mouse_event("MOUSE_UP", ev, buf);
                    case ws::UIE_MOUSE_SCROLL:
                        snprintf(buf, sizeof(buf), " direction=%d, state=0x%x", int(ev->nCode), int(ev->nState));
                        return log_mouse_event("MOUSE_SCROLL", ev, buf);
                    case ws::UIE_MOUSE_CLICK:
                        snprintf(buf, sizeof(buf), " button=%d", int(ev->nCode));
                        return log_mouse_event("MOUSE_CLICK", ev, buf);
                    case ws::UIE_MOUSE_DBL_CLICK:
                        snprintf(buf, sizeof(buf), " button=%d", int(ev->nCode));
                        return log_mouse_event("MOUSE_DBL_CLICK", ev, buf);
                    case ws::UIE_MOUSE_TRI_CLICK:
                        snprintf(buf, sizeof(buf), " button=%d", int(ev->nCode));
                        return log_mouse_event("MOUSE_TRI_CLICK", ev, buf);
                    case ws::UIE_MOUSE_MOVE:
                        return log_mouse_event("MOUSE_MOVE", ev);
                    case ws::UIE_MOUSE_IN:
                        return log_mouse_event("MOUSE_IN", ev);
                    case ws::UIE_MOUSE_OUT:
                        return log_mouse_event("MOUSE_OUT", ev);

                    // Keyboard events
                    case ws::UIE_KEY_DOWN:
                        pTest->printf("KEY_DOWN: code=%d, raw=%d, state=0x%x\n",
                            int(ev->nCode), int(ev->nRawCode), int(ev->nState));
                        break;
                    case ws::UIE_KEY_UP:
                        pTest->printf("KEY_UP: code=%d, raw=%d, state=0x%x\n",
                            int(ev->nCode), int(ev->nRawCode), int(ev->nState));
                        break;

                    // Redraw event
                    case ws::UIE_REDRAW:
                    {
                        pTest->printf("REDRAW\n");

                        Color c(0.0f, 0.5f, 0.75f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(c);
                        s->end();

                        return STATUS_OK;
                    }

                    // Window events
                    case ws::UIE_SIZE_REQUEST:
                        pTest->printf("SIZE_REQUEST: size=(%d, %d)\n",
                            int(ev->nWidth), int(ev->nHeight));
                        return STATUS_OK;
                    case ws::UIE_RESIZE:
                        pTest->printf("RESIZE: coord=(%d, %d), size=(%d, %d)\n",
                            int(ev->nLeft), int(ev->nTop), int(ev->nWidth), int(ev->nHeight));
                        return STATUS_OK;
                    case ws::UIE_FOCUS_IN:
                        pTest->printf("FOCUS_IN\n");
                        return STATUS_OK;
                    case ws::UIE_FOCUS_OUT:
                        pTest->printf("FOCUS_OUT\n");
                        return STATUS_OK;
                    case ws::UIE_SHOW:
                        pTest->printf("SHOW\n");
                        return STATUS_OK;
                    case ws::UIE_HIDE:
                        pTest->printf("HIDE\n");
                        return STATUS_OK;

                    case ws::UIE_CLOSE:
                    {
                        pTest->printf("CLOSE\n");

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

        io::Path font;
        MTEST_ASSERT(font.fmt("%s/font/example.ttf", resources()));
        MTEST_ASSERT(dpy->add_font("example", &font) == STATUS_OK);
        MTEST_ASSERT(dpy->add_font_alias("alias", "example") == STATUS_OK);

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally(
            wnd->destroy();
            delete wnd;
        );
        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test events") == STATUS_OK);
        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_ALL) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(160, 100, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END





