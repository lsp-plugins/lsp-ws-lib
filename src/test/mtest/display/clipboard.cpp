/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 19 июл. 2022 г.
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

#include <lsp-plug.in/io/InMemoryStream.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/ws/factory.h>
#include <lsp-plug.in/ws/IEventHandler.h>

namespace
{
    static const char * const mimes[] =
    {
        "UTF8_STRING",
        "text/plain;charset=utf-8",
        "text/plain;charset=UTF-16LE",
        "text/plain;charset=UTF-16BE",
        "text/plain;charset=US-ASCII",
        "text/plain",
        NULL
    };

    static const char *characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

MTEST_BEGIN("ws.display", clipboard)

    // Clipboard data source
    class CbDataSource: public ws::IDataSource
    {
        private:
            LSPString       sText;
            test_type_t    *pTest;

        public:
            explicit CbDataSource(test_type_t *test, const LSPString &text) : ws::IDataSource(mimes)
            {
                pTest       = test;
                sText.set(&text);
            }

            virtual ~CbDataSource()
            {
            }

        public:
            virtual io::IInStream *open(const char *mime) override
            {
                // Scan supported MIME types
                ssize_t idx = -1, i=0;
                for (const char *const *p = mimes; *p != NULL; ++p, ++i)
                {
                    if (!::strcasecmp(mimes[i], mime))
                    {
                        idx = i;
                        break;
                    }
                }

                // Analyze found MIME type
                void *data      = NULL;
                size_t bytes    = 0;
                switch (idx)
                {
                    case 0: // UTF8_STRING
                    case 1: // text/plain;charset=utf-8
                        data    = sText.clone_utf8(&bytes);
                        bytes  -= sizeof(char);             // 1 extra byte for zero character
                        break;
                    case 2: // text/plain;charset=UTF-16LE
                        data    = __IF_LEBE(
                                sText.clone_utf16(&bytes),
                                sText.clone_native(&bytes, "UTF16-LE")
                            );
                        bytes  -= sizeof(lsp_utf16_t);      // 2 extra bytes for zero character
                        break;
                    case 3: // text/plain;charset=UTF-16BE
                        data = __IF_LEBE(
                                sText.clone_native(&bytes, "UTF16-BE"),
                                sText.clone_utf16(&bytes)
                            );
                        bytes  -= sizeof(lsp_utf16_t);      // 2 extra bytes for zero character
                        break;
                    case 4:
                        data    = sText.clone_ascii(&bytes);
                        bytes  -= sizeof(char);             // 1 extra byte for zero character
                        break;
                    case 5:
                        data    = sText.clone_native(&bytes);
                        bytes  -= sizeof(char);             // 4 extra byte for zero character
                        break;
                    default:
                        break;
                }

                // Format not supported?
                if (data == NULL)
                    return NULL;

                // Allocate memory stream;
                io::InMemoryStream *stream = new io::InMemoryStream(data, bytes, MEMDROP_FREE);
                if (stream == NULL)
                {
                    ::free(data);
                    return NULL;
                }

                return stream;
            }
    };

    // Clipboard data sink
    class CbDataSink: public ws::IDataSink
    {
        protected:
            test_type_t            *pTest;
            io::OutMemoryStream     sOS;
            ssize_t                 nMime;
            const char             *pMime;

        public:
            explicit CbDataSink(test_type_t *test)
            {
                pTest   = test;
                nMime   = -1;
                pMime   = NULL;
            }

            virtual ~CbDataSink()
            {
                clear();
            }

        public:
            inline const char      *mime() const    { return pMime;     }

        public:
            void clear()
            {
                sOS.drop();
                nMime   = -1;
                pMime   = NULL;
            }

            virtual ssize_t open(const char * const *mime_types) override
            {
                ssize_t self_idx = 0, found = -1;

                for (const char *const *p = mimes; (*p != NULL) && (found < 0); ++p, ++self_idx)
                {
                    ssize_t src_idx = 0;
                    for (const char *const *v = mime_types; (*v != NULL) && (found < 0); ++v, ++src_idx)
                    {
                        if (!::strcasecmp(*p, *v))
                        {
                            found   = src_idx;
                            nMime   = self_idx;
                            pMime   = *p;
                            break;
                        }
                    }
                }

                if (found < 0)
                    return -STATUS_UNSUPPORTED_FORMAT;

                pTest->printf("Selected mime type: %s, index=%d\n", pMime, found);
                return found;
            }

            virtual status_t write(const void *buf, size_t count) override
            {
                if (pMime == NULL)
                    return STATUS_CLOSED;
                ssize_t written = sOS.write(buf, count);
                return (written >= ssize_t(count)) ? STATUS_OK : STATUS_UNKNOWN_ERR;
            }

            virtual status_t close(status_t code) override
            {
                if (pMime == NULL)
                {
                    clear();
                    return STATUS_OK;
                }

                // Commit data
                LSPString tmp;

                if (code == STATUS_OK)
                {
                    bool ok     = false;
                    code        = STATUS_NO_MEM;

                    switch (nMime)
                    {
                        case 0: // text/plain;charset=utf-8
                        case 1: // UTF8_STRING
                            ok      = tmp.set_utf8(reinterpret_cast<const char *>(sOS.data()), sOS.size());
                            break;
                        case 2: // text/plain;charset=UTF-16LE
                            ok      =
                                __IF_LEBE(
                                    tmp.set_utf16(reinterpret_cast<const lsp_utf16_t *>(sOS.data())),
                                    tmp.set_native(reinterpret_cast<const char *>(sOS.data()), "UTF16-LE")
                                );
                            break;
                        case 3: // text/plain;charset=UTF-16BE
                            ok      =
                                __IF_LEBE(
                                    tmp.set_native(reinterpret_cast<const char *>(sOS.data()), "UTF16-BE"),
                                    tmp.set_utf16(reinterpret_cast<const lsp_utf16_t *>(sOS.data()))
                                );
                            break;
                        case 4: // text/plain;charset=US-ASCII
                            ok      = tmp.set_ascii(reinterpret_cast<const char *>(sOS.data()));
                            break;
                        case 5: // text/plain
                            ok      = tmp.set_native(reinterpret_cast<const char *>(sOS.data()), sOS.size());
                            break;
                        default:
                            code    = STATUS_UNSUPPORTED_FORMAT;
                            break;
                    }

                    // Successful set?
                    if (ok)
                        code    = STATUS_OK;
                }

                // Drop data
                clear();

                // Submit fetched data to callback method
                if (code == 0)
                    pTest->printf("Received clipboard data:\n%s\n", tmp.get_native());
                else
                    pTest->printf("Failed to receive clipboard data: error=%d\n", int(code));

                return code;
            }
    };

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

            void copy_to_clipboard(size_t id)
            {
                // Generate some random string
                LSPString tmp;
                size_t n = strlen(characters);

                for (size_t r=0; r<8; ++r)
                {
                    for (size_t c=0; c<64; ++c)
                        tmp.append(characters[rand() % n]);
                    tmp.append('\n');
                }

                // Create clpboard data source
                CbDataSource *ds = new CbDataSource(pTest, tmp);
                if (ds == NULL)
                    return;
                ds->acquire();
                lsp_finally{ ds->release(); };

                // Commit data source to clipboard
                pWnd->display()->set_clipboard(id, ds);

                // Output message to console
                pTest->printf("Submitted clipboard data:\n%s\n", tmp.get_native());
            }

            void paste_from_clipboard(size_t id)
            {
                CbDataSink *ds = new CbDataSink(pTest);
                if (ds == NULL)
                    return;
                ds->acquire();
                lsp_finally{ ds->release(); };

                pWnd->display()->get_clipboard(id, ds);
            }

            virtual status_t handle_event(const ws::event_t *ev) override
            {
                switch (ev->nType)
                {
                    case ws::UIE_MOUSE_CLICK:
                    {
                        if (ev->nState & ws::MCF_CONTROL)
                        {
                            if (ev->nCode == ws::MCB_LEFT)
                                copy_to_clipboard(ws::CBUF_PRIMARY);
                            else if (ev->nCode == ws::MCB_RIGHT)
                                copy_to_clipboard(ws::CBUF_CLIPBOARD);
                        }
                        else if (ev->nState & ws::MCF_SHIFT)
                        {
                            if (ev->nCode == ws::MCB_LEFT)
                                paste_from_clipboard(ws::CBUF_PRIMARY);
                            else if (ev->nCode == ws::MCB_RIGHT)
                                paste_from_clipboard(ws::CBUF_CLIPBOARD);
                        }
                        return STATUS_OK;
                    }

                    // Redraw event
                    case ws::UIE_REDRAW:
                    {
                        Color c;
                        c.set_rgb24(0x0088cc);

                        ws::ISurface *s = pWnd->get_surface();
                        if (s == NULL)
                            return STATUS_OK;

                        // Perform drawing
                        s->begin();
                        s->clear(c);
                        s->end();

                        return STATUS_OK;
                    }

                    // Window events
                    case ws::UIE_SIZE_REQUEST:
                        pTest->printf("SIZE_REQUEST: size=(%d, %d)\n",
                            int(ev->nWidth), int(ev->nHeight));
                        return STATUS_OK;
                    case ws::UIE_RESIZE:
                        pTest->printf("RESIZE: coord=(%d, %d), size=(%d, %d)\n",
                            int(ev->nLeft), int(ev->nTop), int(ev->nWidth), int(ev->nHeight));
                        return STATUS_OK;
                    case ws::UIE_FOCUS_IN:
                        pTest->printf("FOCUS_IN\n");
                        return STATUS_OK;
                    case ws::UIE_FOCUS_OUT:
                        pTest->printf("FOCUS_OUT\n");
                        return STATUS_OK;
                    case ws::UIE_SHOW:
                        pTest->printf("SHOW\n");
                        return STATUS_OK;
                    case ws::UIE_HIDE:
                        pTest->printf("HIDE\n");
                        return STATUS_OK;

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

        io::Path font;
        MTEST_ASSERT(font.fmt("%s/font/example.ttf", resources()));
        MTEST_ASSERT(dpy->add_font("example", &font) == STATUS_OK);
        MTEST_ASSERT(dpy->add_font_alias("alias", "example") == STATUS_OK);

        ws::IWindow *wnd = dpy->create_window();
        MTEST_ASSERT(wnd != NULL);
        lsp_finally {
            wnd->destroy();
            delete wnd;
        };
        MTEST_ASSERT(wnd->init() == STATUS_OK);
        MTEST_ASSERT(wnd->set_caption("Test clipboard") == STATUS_OK);
        MTEST_ASSERT(wnd->resize(320, 200) == STATUS_OK);
        MTEST_ASSERT(wnd->set_window_actions(ws::WA_ALL) == STATUS_OK);
        MTEST_ASSERT(wnd->set_size_constraints(160, 100, 640, 400) == STATUS_OK);

        Handler h(this, wnd);
        wnd->set_handler(&h);

        MTEST_ASSERT(wnd->show() == STATUS_OK);
        MTEST_ASSERT(dpy->main() == STATUS_OK);
    }

MTEST_END




