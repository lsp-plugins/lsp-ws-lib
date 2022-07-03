/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 1 июл. 2022 г.
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

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/runtime/system.h>

#include <private/win/WinDisplay.h>
#include <private/win/WinWindow.h>

#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            const WCHAR *WinDisplay::WINDOW_CLASS_NAME = L"lsp-ws-lib window";

            WinDisplay::WinDisplay():
                IDisplay()
            {
                bExit   = false;

                bzero(&sPendingMessage, sizeof(sPendingMessage));
                sPendingMessage.message     = WM_NULL;
            }

            WinDisplay::~WinDisplay()
            {
            }

            status_t WinDisplay::init(int argc, const char **argv)
            {
                bExit   = false;

                WNDCLASSW wc;
                bzero(&wc, sizeof(wc));

                wc.lpfnWndProc   = window_proc;
                wc.hInstance     = GetModuleHandleW(NULL);
                wc.lpszClassName = WINDOW_CLASS_NAME;

                if (!RegisterClassW(&wc))
                {
                    lsp_error("Error registering window class: %ld", long(GetLastError));
                    return STATUS_UNKNOWN_ERR;
                }

                return STATUS_OK;
            }

            void WinDisplay::destroy()
            {
                UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(NULL));
            }

            LRESULT CALLBACK WinDisplay::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                if (uMsg == WM_CREATE)
                {
                    CREATESTRUCTW *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                    WinWindow *wnd = reinterpret_cast<WinWindow *>(create->lpCreateParams);
                    if (wnd != NULL)
                        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }

                // Obtain the "this" pointer
                WinWindow *_this = reinterpret_cast<WinWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

                lsp_trace("Received event: this=%p, hwnd=%p, uMsg=%d(0x%x), wParam=%d(0x%x), lParam=%p",
                    _this, hwnd, int(uMsg), int(uMsg), int(wParam), int(wParam), lParam);

                if (_this == NULL)
                    return DefWindowProcW(hwnd, uMsg, wParam, lParam);

                return _this->process_event(uMsg, wParam, lParam);
            }

            status_t WinDisplay::main()
            {
                system::time_t ts;

                while (!bExit)
                {
                    // Get current time
                    system::get_time(&ts);
                    timestamp_t xts     = (timestamp_t(ts.seconds) * 1000) + (ts.nanos / 1000000);
                    int wtime           = compute_poll_delay(xts, 50); // How many milliseconds to wait

                    // Poll for the new message
                    if (wtime > 0)
                    {
                        // Here we perform a check that there is some message in the queue
                        if (!PeekMessageW(&sPendingMessage, NULL, 0, 0, PM_REMOVE))
                        {
                            UINT_PTR timerId    = SetTimer(NULL, 0, wtime, NULL);
                            BOOL res            = GetMessageW(&sPendingMessage, NULL, 0, 0);
                            KillTimer(NULL, timerId);

                            // Received WM_QUIT message?
                            if (!res)
                            {
                                bExit = true;
                                break;
                            }
                        }
                    }

                    // Do main iteration
                    status_t result = do_main_iteration(xts);
                    if (result != STATUS_OK)
                        return result;
                }

                return STATUS_OK;
            }

            status_t WinDisplay::do_main_iteration(timestamp_t ts)
            {
                // Process all pending messages.
                do
                {
                    if (sPendingMessage.message == WM_QUIT)
                    {
                        bExit = true;
                        return STATUS_OK;
                    }
                    else if (sPendingMessage.message != WM_NULL)
                    {
                        TranslateMessage(&sPendingMessage);
                        DispatchMessageW(&sPendingMessage);
                    }
                } while (PeekMessageW(&sPendingMessage, NULL, 0, 0, PM_REMOVE));

                // At this moment, we don't have any pending messages for processing
                sPendingMessage.message     = WM_NULL;

                // Do main iteration
                status_t result = IDisplay::main_iteration();
                if (result == STATUS_OK)
                    result = process_pending_tasks(ts);

                // Call for main task
                call_main_task(ts);

                // Return number of processed events
                return result;
            }

            status_t WinDisplay::main_iteration()
            {
                // Get current time to determine if need perform a rendering
                system::time_t ts;
                system::get_time(&ts);
                timestamp_t xts = (timestamp_t(ts.seconds) * 1000) + (ts.nanos / 1000000);

                // Do iteration
                return do_main_iteration(xts);
            }

            void WinDisplay::quit_main()
            {
                bExit = true;
            }

            status_t WinDisplay::wait_events(wssize_t millis)
            {
                if (bExit)
                    return STATUS_OK;

                MSG message;
                system::time_t ts;
                system::get_time(&ts);
                timestamp_t xts     = (timestamp_t(ts.seconds) * 1000) + (ts.nanos / 1000000);
                int wtime           = compute_poll_delay(xts, 50); // How many milliseconds to wait

                // Poll for the new message
                if (wtime <= 0)
                    return STATUS_OK;

                // Try to peek message without setting up timer
                if (PeekMessageW(&message, NULL, 0, 0, PM_NOREMOVE))
                    return STATUS_OK;

                // Wait for the specific period of time for the message
                UINT_PTR timerId    = SetTimer(NULL, 0, wtime, NULL);
                BOOL res            = GetMessageW(&sPendingMessage, NULL, 0, 0);
                KillTimer(NULL, timerId);

                // Received WM_QUIT message?
                if (!res)
                    return STATUS_OK;

                return STATUS_OK;
            }

            IWindow *WinDisplay::create_window()
            {
                return new WinWindow(this, NULL, NULL, false);
            }

            IWindow *WinDisplay::create_window(size_t screen)
            {
                return new WinWindow(this, NULL, NULL, false);
            }

            IWindow *WinDisplay::create_window(void *handle)
            {
                return new WinWindow(this, reinterpret_cast<HWND>(handle), NULL, true);
            }

            IWindow *WinDisplay::wrap_window(void *handle)
            {
                return new WinWindow(this, reinterpret_cast<HWND>(handle), NULL, false);
            }

            ISurface *WinDisplay::create_surface(size_t width, size_t height)
            {
                return NULL;
            }

            size_t WinDisplay::screens()
            {
                return 1;
            }

            size_t WinDisplay::default_screen()
            {
                return 0;
            }

            status_t WinDisplay::screen_size(size_t screen, ssize_t *w, ssize_t *h)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const MonitorInfo *WinDisplay::enum_monitors(size_t *count)
            {
                return NULL;
            }

            status_t WinDisplay::set_clipboard(size_t id, IDataSource *ds)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::get_clipboard(size_t id, IDataSink *dst)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const char * const *WinDisplay::get_drag_ctypes()
            {
                return NULL;
            }

            status_t WinDisplay::reject_drag()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::accept_drag(IDataSink *sink, drag_t action, bool internal, const rectangle_t *r)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top)
            {
                POINT p;
                if (!GetCursorPos(&p))
                    return STATUS_UNKNOWN_ERR;

                if (screen != NULL)
                    *screen     = 0;
                if (left != NULL)
                    *left       = p.x;
                if (top != NULL)
                    *top        = p.y;

                return STATUS_OK;
            }

            status_t WinDisplay::add_font(const char *name, const char *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, const io::Path *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, const LSPString *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, io::IInStream *is)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font_alias(const char *name, const char *alias)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::remove_font(const char *name)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            void WinDisplay::remove_all_fonts()
            {
                // TODO
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


