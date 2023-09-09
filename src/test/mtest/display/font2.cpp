/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 мая 2023 г.
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

MTEST_BEGIN("ws.display", font2)

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
                        f.set_name("noto-sans");
                        f.set_size(32);

                        ws::font_parameters_t fp;
                        ws::text_parameters_t tp1, tp2;

                        for (size_t i=0; i<=0x0f; ++i)
                        {
                            f.set_bold(i & 0x1);
                            f.set_italic(i & 0x2);
                            f.set_underline(i & 0x4);
                            f.set_antialias((i & 0x8) ? ws::FA_ENABLED : ws::FA_DISABLED);

                            float x = s->width() * ((i >> 2) * 0.25f + 0.0625f);
                            float y = s->height() * ((i & 0x03) * 0.25f + 0.125f);

                            s->get_font_parameters(f, &fp);
                            s->get_text_parameters(f, &tp1, "Text");
                            s->get_text_parameters(f, &tp2, " Текст");

                            // Text bar 1
                            c.alpha(0.0f);
                            c.set_rgb24(0xffff00);
                            s->fill_rect(c, SURFMASK_NONE, 0, x, y - fp.Ascent, tp1.Width, fp.Height);

                            c.set_rgb24(0xff0000);
                            s->line(c, x + tp1.XBearing, y - fp.Ascent, x + tp1.XBearing, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x00cc00);
                            s->line(c, x + tp1.XAdvance, y - fp.Ascent, x + tp1.XAdvance, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x0000ff);
                            s->line(c, x + tp1.XBearing, y + tp1.YBearing, x + tp1.XAdvance, y + tp1.YBearing, 1.0f);
                            c.set_rgb24(0x00ccff);
                            s->line(c, x + tp1.XBearing, y, x + tp1.XAdvance, y, 1.0f);
                            c.set_rgb24(0xffcc00);
                            s->line(c, x - 8, y - 8, x + 8, y + 8, 1.0f);
                            s->line(c, x - 8, y + 8, x + 8, y - 8, 1.0f);

                            // Output text
                            c.set_rgb24(0x000000);
                            c.alpha(0.25f);
                            s->out_text(f, c, x, y, "Text");

                            // Update position
                            y += 40;

                            // Text bar 2
                            c.alpha(0.0f);
                            c.set_rgb24(0x00ffff);
                            s->fill_rect(c, SURFMASK_NONE, 0, x, y - fp.Ascent, tp2.Width, fp.Height);

                            c.set_rgb24(0xff0000);
                            s->line(c, x + tp2.XBearing, y - fp.Ascent, x + tp2.XBearing, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x00cc00);
                            s->line(c, x + tp2.XAdvance, y - fp.Ascent, x + tp2.XAdvance, y + fp.Descent, 1.0f);
                            c.set_rgb24(0x0000ff);
                            s->line(c, x + tp2.XBearing, y + tp2.YBearing, x + tp2.XAdvance, y + tp2.YBearing, 1.0f);
                            c.set_rgb24(0x00ccff);
                            s->line(c, x + tp2.XBearing, y, x + tp2.XAdvance, y, 1.0f);
                            c.set_rgb24(0xffcc00);
                            s->line(c, x - 8, y - 8, x + 8, y + 8, 1.0f);
                            s->line(c, x - 8, y + 8, x + 8, y - 8, 1.0f);

                            // Output text
                            c.set_rgb24(0x000000);
                            c.alpha(0.25f);
                            s->out_text(f, c, x, y, " Текст");
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

        io::Path font;
        MTEST_ASSERT(font.fmt("%s/font/NotoSansDisplay-Regular.ttf", resources()));
        MTEST_ASSERT(dpy->add_font("noto-sans", &font) == STATUS_OK);

        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test custom font output") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 640, 640, 640) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



