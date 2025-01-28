/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 29 янв. 2025 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/mtest.h>

#define FRAME_PERIOD            (1000 / 25) /* Launch at 25 Hz rate */
#define FRAME_BUFFER_WIDTH      256
#define FRAME_BUFFER_HEIGHT     160

MTEST_BEGIN("ws.display", flame)

    typedef struct osc_t
    {
        float       A0; // Initial amplitude
        float       X0; // Initial location
        float       W0; // Frequency
        float       P0; // Initial phase
        float       R0; // Reduction/Decay
    } osc_t;

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;
            ws::taskid_t    nTaskId;
            size_t          nPhase;
            osc_t           vOsc[3];
            float          *vRow;
            float          *vRGBA;
            uint8_t        *vBuffer;

        protected:
            static  status_t execute_timer(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
            {
                Handler *pthis      = static_cast<Handler *>(arg);
                if (pthis == NULL)
                    return STATUS_OK;

                pthis->nTaskId      = -1;
                pthis->on_timer();
                pthis->launch(time + FRAME_PERIOD);

                return STATUS_OK;
            }

            void oscillate(float *dst, const osc_t *osc, float t, ssize_t n)
            {
                float P = 2.0f * M_PI * osc->W0 * t + osc->P0;

                for (ssize_t x=0; x < n; ++x)
                {
                    float dx = -0.05f * fabs(osc->X0 - x);
                    dst[x] += osc->A0 * cosf(P + dx) * expf(osc->R0 * dx);
                }
            }

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
                nTaskId     = -1;

                nPhase          = 0;

                vOsc[0].A0      = 0.25f;
                vOsc[0].X0      = 64;
                vOsc[0].W0      = 2.0f;
                vOsc[0].P0      = 0.0f;
                vOsc[0].R0      = 0.01f;

                vOsc[1].A0      = 0.25f;
                vOsc[1].X0      = 128;
                vOsc[1].W0      = 6.5f;
                vOsc[1].P0      = 1.0f;
                vOsc[1].R0      = 0.1f;

                vOsc[2].A0      = 0.15f;
                vOsc[2].X0      = 192;
                vOsc[2].W0      = 1.33f;
                vOsc[2].P0      = 0.5f;
                vOsc[2].R0      = 0.05f;

                vRow            = new float[FRAME_BUFFER_WIDTH];
                vRGBA           = new float[FRAME_BUFFER_WIDTH * 4];
                vBuffer         = new uint8_t[FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4];

                bzero(vBuffer, FRAME_BUFFER_WIDTH * FRAME_BUFFER_HEIGHT * 4);
            }

            ~Handler()
            {
                delete [] vBuffer;
                delete [] vRow;
                delete [] vRGBA;
            }

            void on_timer()
            {
                memmove(
                    &vBuffer[FRAME_BUFFER_WIDTH * 4 * 8],
                    vBuffer,
                    FRAME_BUFFER_WIDTH * (FRAME_BUFFER_HEIGHT - 8) * 4);

                dsp::hsla_hue_eff_t eff;
                eff.h       = 0.0f;
                eff.s       = 1.0f;
                eff.l       = 0.5f;
                eff.a       = 0.0f;
                eff.thresh  = 1.0f / 3.0f;

                for (size_t n=0; n<8; ++n)
                {
                    float t      = float(nPhase) / 0x800;
                    dsp::fill(vRow, 0.5f, FRAME_BUFFER_WIDTH);

                    for (size_t i=0; i<3; ++i)
                        oscillate(vRow, &vOsc[i], t, FRAME_BUFFER_WIDTH);

                    dsp::eff_hsla_hue(vRGBA, vRow, &eff, FRAME_BUFFER_WIDTH);
                    dsp::hsla_to_rgba(vRGBA, vRGBA, FRAME_BUFFER_WIDTH);
                    dsp::rgba_to_bgra32(&vBuffer[(7 - n) * FRAME_BUFFER_WIDTH * 4], vRGBA, FRAME_BUFFER_WIDTH);

                    ++nPhase;
                }

                // Query area for redraw
                pWnd->invalidate();
            }

            void launch(ws::timestamp_t deadline = 0)
            {
                nTaskId     = pWnd->display()->submit_task(deadline, execute_timer, this);
            }

            void stop()
            {
                if (nTaskId >= 0)
                    pWnd->display()->cancel_task(nTaskId);
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
                                lsp_finally { delete g; };
                                c.set_rgb24(0x0000ff);
                                g->set_start(c, 0.5f);
                                g->set_stop(c, 0.0f);

                                s->line(g, X(0.0f), Y(-1.0f), X(0.0f), Y(1.0f), 4.0f);
                                s->line(g, X(0.0f), Y(1.0f), X(-0.0625f), Y(1.0f - 0.0625f), 2.0f);
                                s->line(g, X(0.0f), Y(1.0f), X(0.0625f), Y(1.0f - 0.0625f), 2.0f);
                            }
                        }
                        {
                            ws::IGradient *g = s->linear_gradient(X(-1.0f), Y(0.0f), X(1.0f), Y(0.0f));
                            if (g != NULL)
                            {
                                lsp_finally { delete g; };
                                c.set_rgb24(0xff0000);
                                g->set_start(c, 0.5f);
                                g->set_stop(c, 0.0f);

                                s->line(g, X(-1.0f), Y(0.0f), X(1.0f), Y(0.0f), 4.0f);
                                s->line(g, X(1.0f), Y(0.0f), X(1.0f - 0.0625f), Y(0.0625f), 2.0f);
                                s->line(g, X(1.0f), Y(0.0f), X(1.0f - 0.0625f), Y(-0.0625f), 2.0f);
                            }
                        }

                        s->draw_raw(vBuffer,
                            FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, FRAME_BUFFER_WIDTH*4,
                            pWnd->width() * 0.25f, pWnd->height() * 0.25f,
                            pWnd->width() * 0.5f / FRAME_BUFFER_WIDTH, pWnd->height() * 0.5f / FRAME_BUFFER_HEIGHT, 0.0f);

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
        MTEST_ASSERT(wnd->set_caption("Test flame") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        h.launch();
        lsp_finally{ h.stop(); };

        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END



