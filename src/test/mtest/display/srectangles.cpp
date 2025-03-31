/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 1 апр. 2025 г.
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

MTEST_BEGIN("ws.display", srectangles)

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
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Prepare source surface
                        ws::ISurface *src = s->create(64, 64);
                        if (src == NULL)
                            return STATUS_OK;
                        lsp_finally {
                            src->destroy();
                            delete src;
                        };

                        // Draw gradient on a source surface
                        {
                            ws::IGradient *g = src->radial_gradient(32.0f, 32.0f, 32.0f, 32.0f, 48.0f);
                            if (g == NULL)
                                return STATUS_OK;
                            lsp_finally { delete g; };

                            Color c;
                            g->set_start_rgb(0xff00ff);
                            g->set_stop_rgb(0x00ffff);
                            src->begin();
                            {
                                src->fill_rect(g, SURFMASK_NO_CORNER, 0, 0.0f, 0.0f, 64.0f, 64.0f);
                            }
                            src->end();
                        }

                        // Perform drawing
                        Color c(0.0f, 0.5f, 0.75f);
                        s->begin();
                        {
                            s->clear(c);

                            s->draw(src, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

                            for (size_t xi=0; xi<4; ++xi)
                                for (size_t yi=0; yi<4; ++yi)
                                {
                                    const size_t mask = (yi << 2) | xi;

                                    size_t x = 76 * (xi + 1);
                                    size_t y = 76 * (yi + 1);

                                    s->fill_rect(
                                        src, xi * 0.0625f,
                                        mask, 12.0f,
                                        x, y,
                                        src->width(), src->height());
                                }

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
        MTEST_ASSERT(wnd->set_caption("Test surface rectangles") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



