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

MTEST_BEGIN("ws.display", font)

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

                        ws::Font f;
                        ws::font_parameters_t fp;
                        ws::text_parameters_t tp;
                        f.set_name("example");
                        f.set_size(64);

                        s->get_font_parameters(f, &fp);
                        s->get_text_parameters(f, &tp, "A");

                        ssize_t x   = (pWnd->width()  - ssize_t(tp.Width)*2)  >> 1;
                        ssize_t y   = (pWnd->height() - ssize_t(fp.Height)) >> 1;

                        c.set_rgb24(0xffff00);
                        f.set_antialiasing(ws::FA_ENABLED);
                        s->out_text(f, c, x + tp.XBearing, y + fp.Ascent, "A");
                        x += tp.Width;

                        c.set_rgb24(0x00ffff);
                        f.set_name("alias");
                        f.set_antialiasing(ws::FA_DISABLED);
                        s->out_text(f, c, x + tp.XBearing, y + fp.Ascent, "A");

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
        MTEST_ASSERT(wnd->set_caption("Test font") == STATUS_OK);
        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_RESIZE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(160, 100, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END


