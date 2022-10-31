/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 31 окт. 2022 г.
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

#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/ws/IEventHandler.h>

MTEST_BEGIN("ws.display", async_tasks)

    class Handler: public ws::IEventHandler
    {
        private:
            static constexpr const size_t NUM_POINTS = 1000;

        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;
            float           fPhase;

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
                fPhase      = 0.0f;
            }

            void commit_phase(float phase)
            {
                fPhase      = phase;
                // Query area for redraw
                pWnd->invalidate();
            }

            virtual status_t handle_event(const ws::event_t *ev) override
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
                        {
                            lsp_finally { s->end(); };
                            s->clear(c);

                            float *x = new float[NUM_POINTS+1];
                            if (!x)
                                return STATUS_NO_MEM;
                            lsp_finally { delete x; };
                            float *y = new float[NUM_POINTS+1];
                            if (!y)
                                return STATUS_NO_MEM;
                            lsp_finally { delete y; };

                            // Draw lissajous figure
                            for (size_t i=0; i <= NUM_POINTS; ++i)
                            {
                                float a = (i * 2.0f * M_PI) / NUM_POINTS;
                                x[i]    = s->width()  * (1 + cosf(4.0f * a + fPhase)) * 0.5f;
                                y[i]    = s->height() * (1 + sinf(3.0f * a)) * 0.5f;
                            }

                            c.set_rgb(0.0f, 0.0f, 1.0f);
                            s->wire_poly(c, 3, x, y, NUM_POINTS+1);
                        }

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

    typedef struct async_task_t
    {
        Handler        *handler;
        float           phase;
    } async_task_t;

    static status_t async_task_handler(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
    {
        async_task_t *task = static_cast<async_task_t *>(arg);
        if (task == NULL)
            return STATUS_BAD_ARGUMENTS;

        task->handler->commit_phase(task->phase);
        delete task;
        return STATUS_OK;
    }

    class Submitter: public ipc::Thread
    {
        private:
            ws::IWindow        *pWnd;
            Handler            *pHandler;
            uint32_t            nCounter;

        public:
            explicit Submitter(ws::IWindow *wnd, Handler *h)
            {
                pWnd            = wnd;
                pHandler        = h;
                nCounter        = 0;
            };

        public:
            virtual status_t run() override
            {
                while (!is_cancelled())
                {
                    // Submit asynchronous task
                    async_task_t *task = new async_task_t();
                    if (task == NULL)
                        return STATUS_NO_MEM;

                    task->handler   = pHandler;
                    task->phase     = ((nCounter++) & 0x03ff) * 2.0f * M_PI / float(0x400);

                    pWnd->display()->submit_task(0, async_task_handler, task);

                    // Sleep for a while
                    Thread::sleep(20);
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
        MTEST_ASSERT(wnd->set_caption("Test async tasks") == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_MOVE | ws::WA_CLOSE) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(640, 400, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(!wnd->has_parent());

        Submitter submitter(wnd, &h);
        MTEST_ASSERT(submitter.start() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
        MTEST_ASSERT(submitter.cancel() == STATUS_OK);
        MTEST_ASSERT(submitter.join() == STATUS_OK);
    }

MTEST_END




