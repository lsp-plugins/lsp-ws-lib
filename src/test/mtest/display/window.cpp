/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 мая 2020 г.
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

MTEST_BEGIN("ws.display", window)

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

        // Enumerate list of displays
        printf("List of attached displays:\n");
        printf("%2s %10s %10s %s %s\n", "id", "coord", "size", "p", "name");
        size_t count;
        const ws::MonitorInfo *mi = dpy->enum_monitors(&count);
        MTEST_ASSERT(count > 0);
        MTEST_ASSERT(mi != NULL);
        for (size_t i=0; i < count; ++i, ++mi)
        {
            char pos[32], size[32];
            snprintf(pos, sizeof(pos), "%d,%d", int(mi->rect.nLeft), int(mi->rect.nTop));
            snprintf(size, sizeof(size), "%dx%d", int(mi->rect.nWidth), int(mi->rect.nHeight));
            printf("%2d %10s %10s %c %s\n",
                int(i), pos, size, (mi->primary) ? '*' : ' ', mi->name.get_native());
        }
        printf("\n");

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };
        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_mouse_pointer(ws::MP_HAND) == STATUS_OK);
        MTEST_ASSERT(wnd->get_mouse_pointer() == ws::MP_HAND);

        LSPString dst;
        MTEST_ASSERT(wnd->set_caption("Test window") == STATUS_OK);
        MTEST_ASSERT(wnd->get_caption(&dst) == STATUS_OK);
        MTEST_ASSERT(dst.equals_ascii("Test window"));
        MTEST_ASSERT(wnd->set_border_style(ws::BS_DIALOG) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_RESIZE | ws::WA_CLOSE) == STATUS_OK);

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

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END


