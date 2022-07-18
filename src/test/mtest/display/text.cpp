/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 11 июл. 2022 г.
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

MTEST_BEGIN("ws.display", text)

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

                        // Draw the text
                        ws::Font f;
                        f.set_name("arial");
                        f.set_size(32);

                        ws::font_parameters_t fp;
                        ws::text_parameters_t tp;

                        for (size_t i=0; i<=0x07; ++i)
                        {
                            f.set_bold(i & 0x1);
                            f.set_italic(i & 0x2);
                            f.set_underline(i & 0x4);

                            float x = s->width() * ((i >> 2) * 0.5f + 0.25f);
                            float y = s->height() * ((i & 0x03) * 0.25f + 0.125f);

                            s->get_font_parameters(f, &fp);
                            s->get_text_parameters(f, &tp, "Text");

                            c.alpha(0.0f);
                            c.set_rgb24(0xffff00);
                            s->fill_rect(c, SURFMASK_NONE, 0, x, y - fp.Ascent, tp.Width, fp.Height);

                            c.set_rgb24(0xff0000);
                            s->line(c, x + tp.XBearing, y - fp.Ascent, x + tp.XBearing, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x00cc00);
                            s->line(c, x + tp.XAdvance, y - fp.Ascent, x + tp.XAdvance, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x0000ff);
                            s->line(c, x + tp.XBearing, y + tp.YBearing, x + tp.XAdvance, y + tp.YBearing, 1.0f);
                            c.set_rgb24(0x00ccff);
                            s->line(c, x + tp.XBearing, y, x + tp.XAdvance, y, 1.0f);
                            c.set_rgb24(0xffcc00);
                            s->line(c, x - 8, y - 8, x + 8, y + 8, 1.0f);
                            s->line(c, x - 8, y + 8, x + 8, y - 8, 1.0f);

                            // Output text
                            c.set_rgb24(0x000000);
                            c.alpha(0.25f);
                            s->out_text(f, c, x, y, "Text");
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
        lsp_finally { ws::lsp_ws_free_display(dpy); };

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test text") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(400, 640, 400, 640) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END




