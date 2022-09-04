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
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/mtest.h>

MTEST_BEGIN("ws.display", surface)

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

                        ws::ISurface *x = s->create(160, 100);
                        if (x != NULL)
                        {
                            lsp_finally {
                                x->destroy();
                                delete x;
                            };

                            x->begin();
                                c.set_rgba32(0x8800ccff);
                                x->fill_rect(c, SURFMASK_NONE, 0.0f, 0.0f, 0.0f, 80.0f, 100.0f);
                                c.set_rgb24(0xccff00);
                                x->line(c, 0.0f, 0.0f, 160.0f, 100.0f, 2.0f);
                                x->line(c, 0.0f, 100.0f, 160.0f, 0.0f, 2.0f);
                                c.set_rgb24(0x888888);
                                x->wire_rect(c, SURFMASK_NONE, 0.0f, 0.0f, 0.0f, 160, 100, 1.0f);
                            x->end();

                            s->draw(x, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f);
                            s->draw(x, 320.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                            s->draw(x, 320.0f, 100.0f, 1.0f, 1.0f, 0.5f);
                            s->draw_clipped(x, 480.0f, 0.0f, 16, 10, 128, 80, 0.0f);
                            s->draw_rotate(x, 320, 240, 1.0f, 1.0f, M_PI * 0.5f, 0.5f);
                            s->draw(x, 320, 200, 1.5f, 1.5f, 0.0f);
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
        MTEST_ASSERT(wnd->set_caption("Test rectangles") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



