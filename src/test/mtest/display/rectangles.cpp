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

MTEST_BEGIN("ws.display", rectangles)

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
                        Color c(0.0f, 0.5f, 0.75f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(c);

                        ssize_t y = 8;

                        // Filled rectangles
                        // Method 1: rounded solid rectangles
                        c.set_rgb24(0xff0000);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            s->fill_rect(c, i, 8.0f, 8 + 40*i, y, 32, 40);
                        }
                        y += 48;

                        // Method 2: rounded solid rectangles
                        c.set_rgb24(0x00ff00);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            ws::rectangle_t r;
                            r.nLeft     = 8 + 40*i;
                            r.nTop      = y;
                            r.nWidth    = 32;
                            r.nHeight   = 40;
                            s->fill_rect(c, i, 8.0f, &r);
                        }
                        y += 48;

                        // Method 3: linear gradient rectangles
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->linear_gradient(8 + 40*i, y, 8 + 40*(i+1), y + 40);
                            lsp_finally( delete g; );
                            c.set_rgb24(0x0000ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0xffff00);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            s->fill_rect(g, i, 8.0f, 8 + 40*i, y, 32, 40);
                        }
                        y += 48;

                        // Method 4: radial gradient rectangles
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->radial_gradient(8 + 40*i + 16, y + 20, 4.0f, 8 + 40*i + 16, y + 20, 20);
                            lsp_finally( delete g; );
                            c.set_rgb24(0xff00ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0x00ffff);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            ws::rectangle_t r;
                            r.nLeft     = 8 + 40*i;
                            r.nTop      = y;
                            r.nWidth    = 32;
                            r.nHeight   = 40;
                            s->fill_rect(g, i, 8.0f, &r);
                        }
                        y += 48;

                        // Wired rectangles
                        // Method 1: rounded solid rectangles
                        c.set_rgb24(0xff0000);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            s->wire_rect(c, i, 8.0f, 8 + 40*i, y, 32, 40, 1 + (i >> 2));
                        }
                        y += 48;

                        // Method 2: rounded solid rectangles
                        c.set_rgb24(0x00ff00);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            ws::rectangle_t r;
                            r.nLeft     = 8 + 40*i;
                            r.nTop      = y;
                            r.nWidth    = 32;
                            r.nHeight   = 40;
                            s->wire_rect(c, i, 8.0f, &r, 1 + (i >> 2));
                        }
                        y += 48;

                        // Method 3: linear gradient rectangles
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->linear_gradient(8 + 40*i, y, 8 + 40*(i+1), y + 40);
                            lsp_finally( delete g; );
                            c.set_rgb24(0x0000ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0xffff00);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            s->wire_rect(g, i, 8.0f, 8 + 40*i, y, 32, 40, 1 + (i >> 2));
                        }
                        y += 48;

                        // Method 4: radial gradient rectangles
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->radial_gradient(8 + 40*i, y, 8.0f, 8 + 40*i, y, 48);
                            lsp_finally( delete g; );
                            c.set_rgb24(0xff00ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0x00ffff);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            ws::rectangle_t r;
                            r.nLeft     = 8 + 40*i;
                            r.nTop      = y;
                            r.nWidth    = 32;
                            r.nHeight   = 40;
                            s->wire_rect(g, i, 8.0f, &r, 1 + (i >> 2));
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
        lsp_finally( ws::lsp_ws_free_display(dpy); );

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally(
            wnd->destroy();
            delete wnd;
        );

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


