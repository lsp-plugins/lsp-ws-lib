/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 22 июл. 2022 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>

namespace
{
    static const char * const accept_mime[] =
    {
        "text/uri-list",
        "text/x-moz-url",
        "application/x-kde4-urilist",
        "text/plain",
        "application/x-windows-filenamew",
        "application/x-windows-filename",
        NULL
    };
}

MTEST_BEGIN("ws.display", dnd)

    class DragInSink: public ws::IDataSink
    {
        protected:
            enum ctype_t
            {
                TEXT_URI_LIST,
                TEXT_X_MOZ_URL,
                APPLICATION_X_KDE4_URILIST,
                TEXT_PLAIN
            };

        protected:
            test_type_t            *pTest;
            io::OutMemoryStream     sOS;
            ssize_t                 nCtype;

        protected:
            ssize_t                 get_mime_index(const char *mime);

        public:
            explicit DragInSink(test_type_t *test)
            {
                pTest       = test;
                nCtype      = -1;
            }

            virtual ~DragInSink()
            {
            }

        public:
            ssize_t                 select_mime_type(const char * const *mime_types)
            {
                nCtype      = 0;
                for (const char * const *p = accept_mime; *p != NULL; ++p, ++nCtype)
                {
                    ssize_t idx = 0;
                    for (const char * const *q = mime_types; *q != NULL; ++q, ++idx)
                    {
                        if (!::strcasecmp(*p, *q))
                            return idx;
                    }
                }
                return -1;
            }

        public:
            virtual ssize_t         open(const char * const *mime_types) override
            {
                ssize_t ctype   = select_mime_type(mime_types);
                if (ctype < 0)
                    return -STATUS_UNSUPPORTED_FORMAT;

                sOS.clear();
                return ctype;
            }

            virtual status_t        write(const void *buf, size_t count) override
            {
                wssize_t written = sOS.write(buf, count);
                if (written < 0)
                    return -written;
                return (written == wssize_t(count)) ? STATUS_OK : STATUS_UNKNOWN_ERR;
            }

            virtual status_t        close(status_t code) override
            {
                if (code != STATUS_OK)
                {
                    pTest->printf("Failed drop: code=%d\n", int(code));
                    return code;
                }

                pTest->printf("Received content type: %s\n", accept_mime[nCtype]);
                lsp_dumpb("Dump", sOS.data(), sOS.size());

                return STATUS_OK;
            }
    };

    class Handler: public ws::IEventHandler
    {
        private:
            test_type_t    *pTest;
            ws::IWindow    *pWnd;
            bool            bDragOn;
            DragInSink     *pSink;

        public:
            inline Handler(test_type_t *test, ws::IWindow *wnd)
            {
                pTest       = test;
                pWnd        = wnd;
                bDragOn     = false;
                pSink       = new DragInSink(pTest);
                pSink->acquire();
            }

            virtual ~Handler()
            {
                if (pSink != NULL)
                {
                    pSink->release();
                    pSink = NULL;
                }
            }

        public:
            virtual status_t handle_event(const ws::event_t *ev) override
            {
                switch (ev->nType)
                {
                    case ws::UIE_DRAG_ENTER:
                    {
                        pTest->printf("DRAG_ENTER\n");
                        bDragOn     = true;
                        pWnd->invalidate();
                        break;
                    }

                    case ws::UIE_DRAG_REQUEST:
                    {
                        pTest->printf("DRAG_REQUEST x=%d y=%d\n", int(ev->nLeft), int(ev->nTop));
                        ws::IDisplay *dpy = pWnd->display();

                        const char * const *ctype = dpy->get_drag_ctypes();
                        for (const char * const *t = ctype; (t != NULL) && (*t != NULL); ++t)
                            pTest->printf("  %s\n", *t);

                        ssize_t w = pWnd->width()  / 4;
                        ssize_t h = pWnd->height() / 4;

                        if ((ev->nLeft >= w) && (ev->nLeft < w*3) &&
                            (ev->nTop >= h) && (ev->nTop < h*3))
                        {
                            ssize_t idx = pSink->select_mime_type(ctype);
                            if (idx >= 0)
                            {
                                ws::rectangle_t r;
                                r.nLeft     = w;
                                r.nTop      = h;
                                r.nWidth    = w*2;
                                r.nHeight   = h*2;

                                dpy->accept_drag(pSink, ws::DRAG_COPY, &r);
                                pTest->printf("Accepted drag of %s\n", ctype[idx]);
                                break;
                            }
                        }

                        dpy->reject_drag();
                        pTest->printf("Rejected drag\n");
                        break;
                    }

                    case ws::UIE_DRAG_LEAVE:
                    {
                        pTest->printf("DRAG_LEAVE\n");
                        bDragOn     = false;
                        pWnd->invalidate();
                        break;
                    }

                    // Redraw event
                    case ws::UIE_REDRAW:
                    {
                        Color c(0.0f, 0.0f, 0.0f);
                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        size_t w = pWnd->width()  / 4;
                        size_t h = pWnd->height() / 4;

                        // Perform drawing
                        s->begin();
                        s->clear(c);

                        // Clear with blue
                        c.set_rgb24(0x0088cc);
                        s->clear(c);

                        // Draw the Drag & Drop area
                        c.set_rgb24((bDragOn) ? 0xffee00 : 0xcc8800);
                        s->fill_rect(c, SURFMASK_NONE, 0.0f, w, h, w*2, h*2);

                        s->end();

                        return STATUS_OK;
                    }

                    case ws::UIE_CLOSE:
                    {
                        pTest->printf("CLOSE\n");

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
        MTEST_ASSERT(wnd->set_caption("Test drag&drop") == STATUS_OK);
        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_ALL) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(160, 100, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END





