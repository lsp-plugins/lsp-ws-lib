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

MTEST_BEGIN("ws.display", graph)

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

                        float HW    = pWnd->width()  * 0.5f;
                        float HH    = pWnd->height() * 0.5f;
                        #define X(x)  (((x) + 1.0f) * HW)
                        #define Y(y)  ((1.0f - (y)) * HH)

                        // Perform drawing
                        s->begin();
                        s->clear(c);

                        // Thin markers
                        c.set_rgb24(0xcccccc);
                        s->line(c, X(-1.0f), Y(-0.5f), X(1.0f), Y(-0.5f), 1.0f);
                        s->line(c, X(-1.0f), Y(0.5f), X(1.0f), Y(0.5f), 1.0f);
                        s->line(c, X(-0.5f), Y(-1.0f), X(-0.5f), Y(1.0f), 1.0f);
                        s->line(c, X(0.5f), Y(-1.0f), X(0.5f), Y(1.0f), 1.0f);

                        // Axes
                        {
                            ws::IGradient *g = s->linear_gradient(X(0.0f), Y(-1.0f), X(0.0f), Y(1.0f));
                            if (g != NULL)
                            {
                                lsp_finally( delete g; );
                                c.set_rgb24(0x0000ff);
                                g->add_color(0.0f, c, 0.5f);
                                g->add_color(1.0f, c, 0.0f);

                                s->line(g, X(0.0f), Y(-1.0f), X(0.0f), Y(1.0f), 4.0f);
                                s->line(g, X(0.0f), Y(1.0f), X(-0.0625f), Y(1.0f - 0.0625f), 2.0f);
                                s->line(g, X(0.0f), Y(1.0f), X(0.0625f), Y(1.0f - 0.0625f), 2.0f);
                            }
                        }
                        {
                            ws::IGradient *g = s->linear_gradient(X(-1.0f), Y(0.0f), X(1.0f), Y(0.0f));
                            if (g != NULL)
                            {
                                lsp_finally( delete g; );
                                c.set_rgb24(0xff0000);
                                g->add_color(0.0f, c, 0.5f);
                                g->add_color(1.0f, c, 0.0f);

                                s->line(g, X(-1.0f), Y(0.0f), X(1.0f), Y(0.0f), 4.0f);
                                s->line(g, X(1.0f), Y(0.0f), X(1.0f - 0.0625f), Y(0.0625f), 2.0f);
                                s->line(g, X(1.0f), Y(0.0f), X(1.0f - 0.0625f), Y(-0.0625f), 2.0f);
                            }
                        }
                        // Parametric lines
                        c.set_rgb24(0xffff00);
                        {
                            float X0 = X(0.0f);
                            float Y0 = Y(0.25f);
                            float X1 = X(1.0f);
                            float Y1 = Y(1.25f);
                            float A = 1.0f;
                            float B = -A*(X1 - X0) / (Y1 - Y0);
                            float C = -A*X0 - B*Y0;

                            s->parametric_line(c, A, B, C, 2.0f);
                        }
                        c.set_rgb24(0xffff00);
                        {
                            float X0 = X(0.0f);
                            float Y0 = Y(-0.25f);
                            float X1 = X(1.0f);
                            float Y1 = Y(-1.25f);
                            float A = 1.0f;
                            float B = -A*(X1 - X0) / (Y1 - Y0);
                            float C = -A*X0 - B*Y0;

                            s->parametric_line(c, A, B, C, X(-0.75f), X(0.75f), Y(0.75f), Y(-0.75f), 2.0f);
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
        MTEST_ASSERT(wnd->set_caption("Test graph") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



