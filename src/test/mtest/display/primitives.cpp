/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 6 июл. 2022 г.
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

MTEST_BEGIN("ws.display", primitives)

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

                        // Method 1: Filled solid sectors
                        c.set_rgb24(0xff0000);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            s->fill_sector(
                                c, 8 + 40*i, y + 16, 16, i * M_PI / 16.0f, (i + 1) * M_PI / 8.0f);
                        }
                        y += 40;

                        // Method 2: Filled solid triangles
                        c.set_rgb24(0x00ff00);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.alpha(i * 0.0625f);
                            float a  = M_PI * i / 8.0f;
                            float x0 = 16 * cosf(a), y0 = 16 * sinf(a);
                            float x1 = 16 * cosf(a + M_PI * 2.0f / 3.0f), y1 = 16 * sinf(a + M_PI * 2.0f / 3.0f);
                            float x2 = 16 * cosf(a + M_PI * 4.0f / 3.0f), y2 = 16 * sinf(a + M_PI * 4.0f / 3.0f);
                            s->fill_triangle(c,
                                x0 + 24.0f + 40*i, y0 + y + 16,
                                x1 + 24.0f + 40*i, y1 + y + 16,
                                x2 + 24.0f + 40*i, y2 + y + 16);
                        }
                        y += 40;

                        // Method 3: Filled gradient triangles
                        c.set_rgb24(0x00ff00);
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->linear_gradient(8 + 40*i, y, 8 + 40*(i+1), y + 40);
                            if (g == NULL)
                                continue;
                            lsp_finally( delete g; );
                            c.set_rgb24(0x0000ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0xffff00);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            float a  = M_PI * i / 8.0f + M_PI / 2.0f;
                            float x0 = 16 * cosf(a), y0 = 16 * sinf(a);
                            float x1 = 16 * cosf(a + M_PI * 2.0f / 3.0f), y1 = 16 * sinf(a + M_PI * 2.0f / 3.0f);
                            float x2 = 16 * cosf(a + M_PI * 4.0f / 3.0f), y2 = 16 * sinf(a + M_PI * 4.0f / 3.0f);
                            s->fill_triangle(g,
                                x0 + 24.0f + 40*i, y0 + y + 16,
                                x1 + 24.0f + 40*i, y1 + y + 16,
                                x2 + 24.0f + 40*i, y2 + y + 16);
                        }
                        y += 40;

                        // Method 4: Filled solid circles
                        c.set_rgb24(0xffffff);
                        for (size_t i=0; i<16; ++i)
                        {
                            float r = 12 + 4 * cosf(M_PI * i / 8.0f);
                            c.alpha(i * 0.0625f);
                            s->fill_circle(c, 24.0f + 40 * i, y + 16, r);
                        }
                        y += 40;

                        // Method 5: Filled gradient circles
                        c.set_rgb24(0xff00ff);
                        for (size_t i=0; i<16; ++i)
                        {
                            ws::IGradient *g = s->radial_gradient(8 + 40*i + 16, y + 16, 8 + 40*i + 16, y + 16, 20);
                            if (g == NULL)
                                continue;
                            lsp_finally( delete g; );
                            c.set_rgb24(0xff00ff);
                            c.alpha(i * 0.0625f);
                            g->add_color(0.0f, c);
                            c.set_rgb24(0x00ffff);
                            c.alpha(i * 0.0625f);
                            g->add_color(1.0f, c);

                            float r = 12 + 4 * sinf(M_PI * i / 8.0f);
                            s->fill_circle(g, 24.0f + 40 * i, y + 16, r);
                        }
                        y += 40;

                        // Method 6: Wired arcs
                        c.set_rgb24(0xff0000);
                        for (size_t i=0; i<16; ++i)
                        {
                            c.set_rgb24(0x000000);
                            c.alpha(i * 0.0625f);

                            float w = 1 + (i >> 2);
                            s->wire_arc(
                                c, 8 + 40*i, y + 16, 16, i * M_PI / 16.0f, (i + 1) * M_PI / 8.0f, w);
                        }
                        y += 40;

//                        virtual void wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width) override;

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
        MTEST_ASSERT(wnd->set_caption("Test primitives") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END





