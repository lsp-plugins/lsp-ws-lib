/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 28 мар. 2025 г.
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

MTEST_BEGIN("ws.display", origin)

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

                        // Center: circle
                        const ssize_t dw = s->width() / 4;
                        const ssize_t dh = s->height() / 4;

                        c.set_rgb24(0xff0000);
                        const ws::point_t origin = s->set_origin(dw * 2, dh * 2);
                        lsp_finally {
                            s->set_origin(origin);
                        };
                        s->fill_circle(c, 0.0f, 0.0f, 32);

                        // Left top: triangle
                        c.set_rgb24(0x00ff00);
                        s->set_origin(dw, dh);
                        {
                            s->fill_triangle(c,
                                32.0f, 0.0f,
                                cosf(2.0f * M_PI / 3.0f) * 32.0f, sinf(2.0f * M_PI / 3.0f) * 32.0f,
                                cosf(4.0f * M_PI / 3.0f) * 32.0f, sinf(4.0f * M_PI / 3.0f) * 32.0f);
                        }

                        // Right top: solid quad
                        c.set_rgb24(0x0000ff);
                        s->set_origin(dw * 3, dh);
                        s->fill_rect(c,
                            SURFMASK_NO_CORNER,
                            0.0f,
                            -32.0f, -32.0f,
                            64.0f, 64.0f);

                        // Left bottom: linear gradient triangle
                        s->set_origin(dw, dh * 3);
                        ws::IGradient *g = s->linear_gradient(-16.0f, -16.0f, 16.0f, 16.0f);
                        if (g != NULL)
                        {
                            lsp_finally {
                                delete g;
                                g = NULL;
                            };

                            c.set_rgb24(0x0000ff);
                            c.alpha(0.25f);
                            g->set_start(c);
                            c.set_rgb24(0xffff00);
                            c.alpha(0.0f);
                            g->set_stop(c);

                            s->fill_triangle(g,
                                32.0f, 0.0f,
                                cosf(2.0f * M_PI / 3.0f) * 32.0f, sinf(2.0f * M_PI / 3.0f) * 32.0f,
                                cosf(4.0f * M_PI / 3.0f) * 32.0f, sinf(4.0f * M_PI / 3.0f) * 32.0f);
                        }

                        // Right bottom: radial gradient quad
                        s->set_origin(dw * 3, dh * 3);

                        g = s->radial_gradient(-8.0f, -8.0f, -8.0f, -8.0f, 64.0f);
                        if (g != NULL)
                        {
                            lsp_finally {
                                delete g;
                                g = NULL;
                            };

                            c.set_rgb24(0xff00ff);
                            c.alpha(0.0f);
                            g->set_start(c);
                            c.set_rgb24(0x00ffff);
                            c.alpha(0.25f);
                            g->set_stop(c);

                            s->fill_rect(g,
                                SURFMASK_NO_CORNER,
                                0.0f,
                                -32.0f, -32.0f,
                                64.0f, 64.0f);
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
        MTEST_ASSERT(wnd->set_caption("Test surface origin") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END




