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
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/ws/win/decode.h>

#include <private/win/WinDisplay.h>
#include <private/win/WinWindow.h>
#include <private/win/com.h>
#include <private/win/dnd.h>
#include <private/win/fonts.h>
#include <private/win/util.h>

#include <combaseapi.h>
#include <locale.h>
#include <windows.h>
#include <d2d1.h>

// Define the placement-new for our construction/destruction tricks
inline void *operator new (size_t size, void *ptr)
{
    return ptr;
}

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            //-----------------------------------------------------------------
            static const BYTE none_cursor_and[] = { 0xff };
            static const BYTE none_cursor_xor[] = { 0 };

            //-----------------------------------------------------------------
            const WCHAR *WinDisplay::WINDOW_CLASS_NAME      = L"lsp-ws-lib window";
            const WCHAR *WinDisplay::CLIPBOARD_CLASS_NAME   = L"lsp-ws-lib clipboard window";
            volatile atomic_t WinDisplay::hLock             = 0;
            volatile DWORD WinDisplay::nThreadId            = 0;
            WinDisplay  *WinDisplay::pHandlers              = NULL;
            HHOOK WinDisplay::hMouseHook                    = NULL;
            HHOOK WinDisplay::hKeyboardHook                 = NULL;

            //-----------------------------------------------------------------
            WinDisplay::WinDisplay():
                IDisplay()
            {
                bExit           = false;
                pD2D1Factroy    = NULL;
                pWICFactory     = NULL;
                pDWriteFactory  = NULL;
                hWindowClass    = 0;
                hClipClass      = 0;

                bzero(&sPendingMessage, sizeof(sPendingMessage));
                sPendingMessage.message     = WM_NULL;

                for (size_t i=0; i<__MP_COUNT; ++i)
                    vCursors[i]     = NULL;

                pNextHandler    = NULL;
                hClipWnd        = NULL;
                pClipData       = NULL;
                pDragWindow     = NULL;
                pPingThread     = NULL;
            }

            WinDisplay::~WinDisplay()
            {
                do_destroy();
            }

            status_t WinDisplay::init(int argc, const char **argv)
            {
                HRESULT hr;
                WNDCLASSW wc;

                hr = OleInitialize(NULL);
                if (FAILED(hr))
                    return STATUS_UNKNOWN_ERR;

                bExit           = false;

                // Register window class
                bzero(&wc, sizeof(wc));

                wc.lpfnWndProc   = window_proc;
                wc.hInstance     = GetModuleHandleW(NULL);
                wc.lpszClassName = WINDOW_CLASS_NAME;

                if (!(hWindowClass = RegisterClassW(&wc)))
                {
                    lsp_error("Error registering window class: %ld", long(GetLastError));
                    return STATUS_UNKNOWN_ERR;
                }

                // Register clipboard window class
                bzero(&wc, sizeof(wc));

                wc.lpfnWndProc   = clipboard_proc;
                wc.hInstance     = GetModuleHandleW(NULL);
                wc.lpszClassName = CLIPBOARD_CLASS_NAME;

                if (!(hClipClass = RegisterClassW(&wc)))
                {
                    lsp_error("Error registering clipboard window class: %ld", long(GetLastError));
                    return STATUS_UNKNOWN_ERR;
                }

                // Initialize cursors
                for (size_t i=0; i<__MP_COUNT; ++i)
                    translate_cursor(mouse_pointer_t(i));

                // Create D2D1 factory
                hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2D1Factroy);
                if (FAILED(hr))
                {
                    lsp_error("Error creating D2D1 factory: %ld", long(GetLastError()));
                    return STATUS_UNKNOWN_ERR;
                }

                // Create the Windows Imaging factory
                hr = CoCreateInstance(
                    CLSID_WICImagingFactory,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&pWICFactory)
                );
                if (FAILED(hr))
                {
                    lsp_error("Error creating WIC factory: %ld", long(GetLastError()));
                    return STATUS_UNKNOWN_ERR;
                }

                // Create DirectWrite factory and initialize font cache
                hr = DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&pDWriteFactory));
                if (FAILED(hr))
                {
                    lsp_error("Error creating DirectWrite factory: %ld", long(GetLastError()));
                    return STATUS_UNKNOWN_ERR;
                }
                if (!create_font_cache())
                {
                    lsp_error("Error initializing font cache");
                    return STATUS_UNKNOWN_ERR;
                }
                lsp_debug("Default font family: %s", sDflFontFamily.get_native());

                // Create invisible clipboard window
                hClipWnd = CreateWindowExW(
                        0,                                  // dwExStyle
                        WinDisplay::CLIPBOARD_CLASS_NAME,   // lpClassName
                        L"Clipboard",                       // lpWindowName
                        WS_OVERLAPPEDWINDOW,                // dwStyle
                        0,                                  // X
                        0,                                  // Y
                        1,                                  // nWidth
                        1,                                  // nHeight
                        NULL,                               // hWndParent
                        NULL,                               // hMenu
                        GetModuleHandleW(NULL),             // hInstance
                        this);                              // lpCreateParam
                if (hClipWnd == NULL)
                {
                    lsp_error("Error creating clipboard window: %ld", long(GetLastError()));
                    return STATUS_UNKNOWN_ERR;
                }

                // Create pinging thread
                pPingThread = new ipc::Thread(ping_proc, this);
                if (pPingThread == NULL)
                    return STATUS_UNKNOWN_ERR;
                pPingThread->start();

                return IDisplay::init(argc, argv);
            }

            void WinDisplay::do_destroy()
            {
                // Terminate the ping thread
                if (pPingThread != NULL)
                {
                    lsp_trace("Stopping ping thread");
                    pPingThread->cancel();
                    pPingThread->join();
                    lsp_trace("Stopped ping thread");

                    delete pPingThread;
                    pPingThread = NULL;
                }

                // Destroy clipboard window
                if (hClipWnd != NULL)
                {
                    // Destroy the window
                    DestroyWindow(hClipWnd);
                    hClipWnd            = NULL;
                }

                // Unregister window classes
                if (hWindowClass != 0)
                {
                    UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(NULL));
                    hWindowClass    = 0;
                }
                if (hClipClass != 0)
                {
                    UnregisterClassW(CLIPBOARD_CLASS_NAME, GetModuleHandleW(NULL));
                    hClipClass    = 0;
                }

                // Destroy cursors
                if (vCursors[MP_NONE] != NULL)
                {
                    DestroyCursor(vCursors[MP_NONE]);
                    vCursors[MP_NONE]   = NULL;
                }
                for (size_t i=0; i<__MP_COUNT; ++i)
                    vCursors[i]         = NULL;

                // Destroy grab state
                for (size_t i=0; i<__GRAB_TOTAL; ++i)
                    vGrab[i].clear();
                uninstall_windows_hooks();

                // Release factories
                safe_release( pDWriteFactory );
                safe_release( pWICFactory );
                safe_release( pD2D1Factroy );

                // Release clipboard data
                destroy_clipboard();

                // Destroy monitors
                drop_monitors(&vMonitors);
                drop_font_cache(&vFontCache);
                remove_all_fonts();
            }

            void WinDisplay::destroy()
            {
                do_destroy();
                IDisplay::destroy();
            }

            status_t WinDisplay::ping_proc(void *arg)
            {
                WinDisplay *_this = static_cast<WinDisplay *>(arg);

                while (!ipc::Thread::is_cancelled())
                {
                    SendMessageW(_this->hClipWnd, WM_USER, 0, 0);
                    ipc::Thread::sleep(20);
                }

                return STATUS_OK;
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
                WinWindow *wnd = reinterpret_cast<WinWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

//                lsp_trace("Received event: this=%p, hwnd=%p, uMsg=%d(0x%x), wParam=%d(0x%x), lParam=%p",
//                    _this, hwnd, int(uMsg), int(uMsg), int(wParam), int(wParam), lParam);
                if (wnd == NULL)
                    return DefWindowProcW(hwnd, uMsg, wParam, lParam);

                timestamp_t ts = GetMessageTime();

//                // Debug
//                switch (uMsg)
//                {
//                    case WM_LBUTTONDOWN:
//                    case WM_RBUTTONDOWN:
//                    case WM_MBUTTONDOWN:
//                    case WM_XBUTTONDOWN:
//                        lsp_trace("MOUSE_DOWN hwnd=%p, uMsg=%d, wParam=0x%lx, lParam=0x%x",
//                            hwnd, int(uMsg), long(wParam), long(lParam));
//                        break;
//                    case WM_LBUTTONUP:
//                    case WM_RBUTTONUP:
//                    case WM_MBUTTONUP:
//                    case WM_XBUTTONUP:
//                        lsp_trace("MOUSE_UP hwnd=%p, uMsg=%d, wParam=0x%lx, lParam=0x%x",
//                            hwnd, int(uMsg), long(wParam), long(lParam));
//                        break;
//                    case WM_MOUSEMOVE:
//                        lsp_trace("MOUSE_MOVE hwnd=%p, uMsg=%d, wParam=0x%lx, lParam=0x%x",
//                            hwnd, int(uMsg), long(wParam), long(lParam));
//                        break;
//                    default:
//                        break;
//                }

                // Do not deliver several events to the window handler if it is grabbing events at this moment via hook
                if (is_hookable_event(uMsg))
                {
                    if (wnd->pWinDisplay->has_grabbing_events())
                    {
                        if (!wnd->is_grabbing_events())
                            return 0;
                    }
                }

                // Deliver event to the window
                return wnd->process_event(uMsg, wParam, lParam, ts, false);
            }

            status_t WinDisplay::main()
            {
                while (!bExit)
                {
                    // Get current time
                    timestamp_t xts     = system::get_time_millis();
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
                size_t limit = 0;
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
                    if ((limit++) >= 0x20)
                        break;
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
                timestamp_t xts = system::get_time_millis();

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
                timestamp_t xts     = system::get_time_millis();
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
                if (screen != default_screen())
                    return STATUS_BAD_ARGUMENTS;

                ssize_t width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                if (width == 0)
                {
                    if (GetLastError() != ERROR_SUCCESS)
                        return STATUS_UNKNOWN_ERR;
                }
                else if (width < 0)
                    return STATUS_UNKNOWN_ERR;

                ssize_t height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                if (height == 0)
                {
                    if (GetLastError() != ERROR_SUCCESS)
                        return STATUS_UNKNOWN_ERR;
                }
                else if (height < 0)
                    return STATUS_UNKNOWN_ERR;

                if (w != NULL)
                    *w      = width;
                if (h != NULL)
                    *h      = height;

                return STATUS_OK;
            }

            void WinDisplay::drop_monitors(lltl::darray<MonitorInfo> *list)
            {
                for (size_t i=0, n=list->size(); i<n; ++i)
                {
                    MonitorInfo *mi = list->uget(i);
                    mi->name.~LSPString();
                }
                list->flush();
            }

            WINBOOL CALLBACK WinDisplay::enum_monitor_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM dwParam)
            {
                lltl::darray<MonitorInfo> *result = reinterpret_cast<lltl::darray<MonitorInfo> *>(dwParam);
                MonitorInfo *di     = result->add();
                if (di == NULL)
                    return TRUE;
                new (static_cast<void *>(&di->name)) LSPString;

                MONITORINFOEXW xmi;
                xmi.cbSize       = sizeof(xmi);

                if (GetMonitorInfoW(monitor, &xmi))
                {
                    di->primary         = xmi.dwFlags & MONITORINFOF_PRIMARY;
                    di->name.set_utf16(xmi.szDevice);
                }
                else
                    di->primary         = false;

                di->rect.nLeft      = rect->left;
                di->rect.nTop       = rect->top;
                di->rect.nWidth     = rect->right - rect->left;
                di->rect.nHeight    = rect->bottom - rect->top;

                return TRUE;
            }

            const MonitorInfo *WinDisplay::enum_monitors(size_t *count)
            {
                lltl::darray<MonitorInfo> result;

                EnumDisplayMonitors(
                    NULL,                                   // hdc
                    NULL,                                   // lprcClip
                    enum_monitor_proc,                      // lpfnEnum
                    reinterpret_cast<LPARAM>(&result)       // dwData
                );

                vMonitors.swap(result);
                drop_monitors(&result);
                if (count != NULL)
                    *count      = vMonitors.size();

                return vMonitors.array();
            }

            const char * const *WinDisplay::get_drag_ctypes()
            {
                WinDNDTarget *tgt = (pDragWindow != NULL) ? pDragWindow->dnd_target() : NULL;
                if (tgt == NULL)
                    return NULL;

                return tgt->formats();
            }

            status_t WinDisplay::reject_drag()
            {
                WinDNDTarget *tgt = (pDragWindow != NULL) ? pDragWindow->dnd_target() : NULL;
                return (tgt != NULL) ? tgt->reject_drag() : STATUS_BAD_STATE;
            }

            status_t WinDisplay::accept_drag(IDataSink *sink, drag_t action, const rectangle_t *r)
            {
                WinDNDTarget *tgt = (pDragWindow != NULL) ? pDragWindow->dnd_target() : NULL;
                return (tgt != NULL) ? tgt->accept_drag(sink, action, r) : STATUS_BAD_STATE;
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

            status_t WinDisplay::add_font(const char *name, io::IInStream *is)
            {
                HRESULT hr;

                if ((name == NULL) || (is == NULL))
                    return STATUS_BAD_ARGUMENTS;
                if (pDWriteFactory == NULL)
                    return STATUS_BAD_STATE;

                // Create font record
                font_t *f = vCustomFonts.get(name);
                if (f != NULL)
                    return STATUS_ALREADY_EXISTS;
                if ((f = alloc_font(name)) == NULL)
                    return STATUS_NO_MEM;
                lsp_finally{ drop_font(f); };

                // Dump data to memory stream
                io::OutMemoryStream os;
                wssize_t length = is->sink(&os);
                if (length < 0)
                    return status_t(-length);

                // Create file loader
                f->file = new WinFontFileLoader(&os);
                if (f->file == NULL)
                    return STATUS_NO_MEM;
                f->file->AddRef();
                hr = pDWriteFactory->RegisterFontFileLoader(f->file);
                if (FAILED(hr))
                    return STATUS_UNKNOWN_ERR;

                // Create collection loader
                f->loader = new WinFontCollectionLoader();
                if (f->loader == NULL)
                    return STATUS_NO_MEM;
                f->loader->AddRef();
                hr = pDWriteFactory->RegisterFontCollectionLoader(f->loader);
                if (FAILED(hr))
                    return STATUS_UNKNOWN_ERR;

                // Create custom font collection
                hr = pDWriteFactory->CreateCustomFontCollection(f->loader, &f->file, sizeof(&f->file), &f->collection);
                if ((FAILED(hr)) || (f->collection == NULL))
                    return STATUS_UNKNOWN_ERR;

                // Obtain the first available font family
                UINT32 count = f->collection->GetFontFamilyCount();
                if (count <= 0)
                    return STATUS_UNKNOWN_ERR;
                f->collection->GetFontFamily(0, &f->family);
                if ((FAILED(hr)) || (f->family == NULL))
                    return STATUS_UNKNOWN_ERR;

                // Obtain the first matching font
                IDWriteLocalizedStrings *names = NULL;
                hr = f->family->GetFamilyNames(&names);
                if ((FAILED(hr)) || (names == NULL))
                    return STATUS_UNKNOWN_ERR;
                lsp_finally{ safe_release(names); };

                // Enumerate all possible font family names
                for (size_t j=0, m=names->GetCount(); j<m; ++j)
                {
                    // Obtain the string with the family name
                    hr = names->GetStringLength(j, &count);
                    if (FAILED(hr))
                        continue;
                    WCHAR *tmp      = reinterpret_cast<WCHAR *>(realloc(f->wname, sizeof(WCHAR) * (count + 2)));
                    if (tmp == NULL)
                        continue;
                    f->wname        = tmp;
                    hr = names->GetString(j, f->wname, count + 2);
                    if (FAILED(hr))
                        continue;

                    // The font has been found, add it to registry and exit
                    if (!vCustomFonts.create(name, f))
                        return STATUS_UNKNOWN_ERR;
                #ifdef LSP_TRACE
                    LSPString out;
                    out.set_utf16(f->wname);
                    lsp_trace("Registered font %s as font family %s", name, out.get_native());
                #endif /* LSP_TRACE */

                    // Do not drop the font record at exit
                    f = NULL;
                    return STATUS_OK;
                }

                return STATUS_UNKNOWN_ERR;
            }

            status_t WinDisplay::add_font_alias(const char *name, const char *alias)
            {
                if ((name == NULL) || (alias == NULL))
                    return STATUS_BAD_ARGUMENTS;

                font_t *f = vCustomFonts.get(name);
                if (f != NULL)
                    return STATUS_ALREADY_EXISTS;

                if ((f = alloc_font(name)) == NULL)
                    return STATUS_NO_MEM;
                if ((f->alias = strdup(alias)) == NULL)
                {
                    drop_font(f);
                    return STATUS_NO_MEM;
                }
                if (!vCustomFonts.create(name, f))
                {
                    drop_font(f);
                    return STATUS_NO_MEM;
                }

                return STATUS_OK;
            }

            status_t WinDisplay::remove_font(const char *name)
            {
                if (name == NULL)
                    return STATUS_BAD_ARGUMENTS;

                font_t *f = NULL;;
                if (!vCustomFonts.remove(name, &f))
                    return STATUS_NOT_FOUND;

                drop_font(f);
                return STATUS_OK;
            }

            WinDisplay::font_t *WinDisplay::alloc_font(const char *name)
            {
                font_t *f = static_cast<font_t *>(malloc(sizeof(font_t)));
                if (f == NULL)
                    return NULL;
                if ((f->name = strdup(name)) == NULL)
                {
                    free(f);
                    return NULL;
                }
                f->file         = NULL;
                f->loader       = NULL;
                f->alias        = NULL;
                f->wname        = NULL;
                f->family       = NULL;
                f->collection   = NULL;
                return f;
            }

            void WinDisplay::drop_font(font_t *f)
            {
                if (f == NULL)
                    return;

                if (f->name != NULL)
                {
                    free(f->name);
                    f->name = NULL;
                }

                if (f->family != NULL)
                {
                    if (f->wname != NULL)
                    {
                        free(f->wname);
                        f->wname = NULL;
                    }
                }
                else
                {
                    if (f->alias != NULL)
                    {
                        free(f->alias);
                        f->alias = NULL;
                    }
                }

                if (f->loader != NULL)
                    pDWriteFactory->UnregisterFontCollectionLoader(f->loader);
                if (f->file != NULL)
                    pDWriteFactory->UnregisterFontFileLoader(f->file);

                safe_release(f->file);
                safe_release(f->loader);
                safe_release(f->family);
                safe_release(f->collection);

                free(f);
            }

            void WinDisplay::remove_all_fonts()
            {
                lltl::parray<font_t> custom;
                vCustomFonts.values(&custom);
                vCustomFonts.flush();

                for (size_t i=0, n=custom.size(); i<n; ++i)
                    drop_font(custom.uget(i));
                custom.flush();
            }

            bool WinDisplay::create_font_cache()
            {
                // Get font collection
                IDWriteFontCollection *fc = NULL;
                HRESULT hr      = pDWriteFactory->GetSystemFontCollection(&fc, FALSE);
                if (FAILED(hr))
                    return false;
                lsp_finally{ safe_release(fc); };

                // Temporary variables to store font family names
                WCHAR *buf  = NULL;
                size_t blen = 0;
                lsp_finally{
                    if (buf != NULL)
                        free(buf);
                };
                LSPString fname, dfl_name;

                // Update the cache
                lsp_trace("Scanning for available fonts");

                for (size_t i=0, n=fc->GetFontFamilyCount(); i<n; ++i)
                {
                    // Get font family
                    IDWriteFontFamily *ff = NULL;
                    hr = fc->GetFontFamily(i, &ff);
                    if ((FAILED(hr)) || (ff == NULL))
                        continue;
                    lsp_finally{ safe_release(ff); };

                    // Obtain names of the font family
                    IDWriteLocalizedStrings *fnames = NULL;
                    hr = ff->GetFamilyNames(&fnames);
                    if ((FAILED(hr)) || (fnames == NULL))
                        continue;
                    lsp_finally{ safe_release(fnames); };

                    // Enumerate all possible font family names
                    for (size_t j=0, m=fnames->GetCount(); j<m; ++j)
                    {
                        // Obtain the string with the family name
                        UINT32 str_len = 0;
                        hr = fnames->GetStringLength(j, &str_len);
                        if (FAILED(hr))
                            continue;
                        if (blen < str_len)
                        {
                            WCHAR *tmp  = reinterpret_cast<WCHAR *>(realloc(buf, sizeof(WCHAR) * (str_len + 2)));
                            if (tmp == NULL)
                                continue;
                            buf         = tmp;
                            blen        = str_len;
                        }
                        hr = fnames->GetString(j, buf, blen + 2);
                        if (FAILED(hr))
                            continue;

                        // Add font family to the cache
                        if (!fname.set_utf16(buf))
                            continue;
                        fname.tolower();
                        lsp_trace("  %s", fname.get_native());
                        if (vFontCache.create(&fname, ff))
                        {
                            if (dfl_name.is_empty())
                                dfl_name.swap(&fname);
                            ff->AddRef();
                        }
                    }
                }

                // Obtain default font family
                // Tier 1
                NONCLIENTMETRICSW metrics;
                metrics.cbSize = sizeof(metrics);
                if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
                {
                    LOGFONTW *list[] = {
                        &metrics.lfMessageFont,
                        &metrics.lfMenuFont,
                        &metrics.lfCaptionFont,
                        &metrics.lfSmCaptionFont,
                        &metrics.lfStatusFont,
                    };

                    for (LOGFONTW *item : list)
                    {
                        if (fname.set_utf16(item->lfFaceName, wcsnlen(item->lfFaceName, LF_FACESIZE)))
                        {
                            fname.tolower();
                            if (vFontCache.contains(&fname))
                            {
                                sDflFontFamily.swap(fname);
                                return true;
                            }
                        }
                    }
                }

                // Tier 2
                static const uint32_t stock_objects[] = {
                    SYSTEM_FONT,
                    DEFAULT_GUI_FONT,
                    DEVICE_DEFAULT_FONT
                };
                for (uint32_t id: stock_objects)
                {
                    HFONT hfont = reinterpret_cast<HFONT>(GetStockObject(id));
                    if (hfont != NULL)
                    {
                        lsp_finally{ DeleteObject(hfont); };

                        LOGFONTW lf;
                        ::bzero(&lf, sizeof(lf));
                        if (GetObjectW(hfont, sizeof(lf), &lf) > 0)
                        {
                            if (fname.set_utf16(lf.lfFaceName, wcsnlen(lf.lfFaceName, LF_FACESIZE)))
                            {
                                fname.tolower();
                                if (vFontCache.contains(&fname))
                                {
                                    sDflFontFamily.swap(fname);
                                    return true;
                                }
                            }
                        }
                    }
                }

                // Tier 3
                if (dfl_name.is_empty())
                    return false;

                sDflFontFamily.swap(dfl_name);
                return true;
            }

            WinDisplay::font_t *WinDisplay::get_custom_font_collection(const char *name)
            {
                lltl::parray<char> visited;

                const char *resolved = name;
                while (true)
                {
                    font_t *f = vCustomFonts.get(resolved);
                    if (f == NULL)
                        return NULL;
                    if (f->family != NULL)
                        return f;
                    if (f->alias == NULL)
                        return NULL;

                    // Prevent from infinite recursion
                    resolved        = f->alias;
                    for (size_t i=0, n=visited.size(); i<n; ++i)
                    {
                        const char *v = visited.uget(i);
                        if (strcmp(resolved, v) == 0)
                            return NULL;
                    }

                    // Append record to visited
                    if (!visited.add(const_cast<char *>(resolved)))
                        return NULL;
                }
            }

            IDWriteFontFamily *WinDisplay::get_font_family(const Font &f, LSPString *name, font_t **custom)
            {
                // Prepare font name
                LSPString tmp;
                if (!tmp.set_utf8(f.name()))
                    return NULL;
                tmp.tolower();

                // Obtain custom font collection
                if (custom != NULL)
                    *custom     = get_custom_font_collection(tmp.get_utf8());

                // Obtain the font family
                IDWriteFontFamily *ff = vFontCache.get(&tmp);
                if (ff != NULL)
                {
                    if (name != NULL)
                        name->swap(tmp);
                    ff->AddRef();
                    return ff;
                }

                // Obtain the default font family
                ff = vFontCache.get(&sDflFontFamily);
                if (ff == NULL)
                    return NULL;
                if (name != NULL)
                {
                    if (!name->set(&sDflFontFamily))
                        return NULL;
                }
                ff->AddRef();
                return ff;
            }

            IDWriteTextLayout *WinDisplay::create_text_layout(
                const Font &f,
                const WCHAR *fname,
                IDWriteFontCollection *fc,
                IDWriteFontFamily *ff,
                const WCHAR *string,
                size_t length)
            {
                HRESULT hr;

                // Obtain the system locale
                const WCHAR *wLocale    = _wsetlocale(LC_ALL, NULL);
                if (wLocale == NULL)
                    wLocale         = L"en-us";

                // Create text format
                IDWriteTextFormat *tf   = NULL;
                hr = pDWriteFactory->CreateTextFormat(
                    fname,                      // Font family name
                    fc,                         // Font collection (NULL sets it to use the system font collection)
                    (f.bold())   ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
                    (f.italic()) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    f.size(),
                    wLocale,
                    &tf);
                if ((FAILED(hr)) || (tf == NULL))
                {
                    if (!f.italic())
                        return NULL;

                    // Create text format with OBLIQUE style instead of Italic
                    hr = pDWriteFactory->CreateTextFormat(
                        fname,
                        fc,
                        (f.bold()) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
                        DWRITE_FONT_STYLE_OBLIQUE,
                        DWRITE_FONT_STRETCH_NORMAL,
                        f.size(),
                        wLocale,
                        &tf);
                    if ((FAILED(hr)) || (tf == NULL))
                        return NULL;
                }
                lsp_finally{ safe_release(tf); };

                tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                tf->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, 0.0f, 0.0f);

                // Create text layout
                IDWriteTextLayout *layout = NULL;
                hr = pDWriteFactory->CreateTextLayout(
                    string,
                    wcslen(string),
                    tf,
                    10000.0f,
                    10000.0f,
                    &layout);
                if ((FAILED(hr)) || (layout == NULL))
                    return NULL;

                // Apply underline option
                DWRITE_TEXT_RANGE range;
                range.startPosition     = 0;
                range.length            = length;
                layout->SetUnderline(f.is_underline(), range);

                return layout;
            }

            void WinDisplay::drop_font_cache(font_cache_t *cache)
            {
                lltl::parray<IDWriteFontFamily> list;
                cache->values(&list);
                cache->flush();
                for (size_t i=0, n=list.size(); i<n; ++i)
                {
                    IDWriteFontFamily *item = list.uget(i);
                    safe_release( item );
                }
            }

            bool WinDisplay::get_font_metrics(const Font &f, IDWriteFontFamily *ff, DWRITE_FONT_METRICS *metrics)
            {
                HRESULT hr;

                // Find the matching font
                DWRITE_FONT_WEIGHT fWeight  = (f.bold())   ? DWRITE_FONT_WEIGHT_BOLD  : DWRITE_FONT_WEIGHT_REGULAR;
                DWRITE_FONT_STYLE fStyle    = (f.italic()) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
                DWRITE_FONT_STRETCH fStretch= DWRITE_FONT_STRETCH_NORMAL;

                IDWriteFont* font = NULL;
                hr = ff->GetFirstMatchingFont(fWeight, fStretch, fStyle, &font);
                if ((FAILED(hr)) || (font == NULL))
                {
                    if (!f.italic())
                        return false;
                    hr = ff->GetFirstMatchingFont(fWeight, fStretch, DWRITE_FONT_STYLE_OBLIQUE, &font);
                    if ((FAILED(hr)) || (font == NULL))
                        return false;
                }
                lsp_finally{ safe_release(font); };

                // Get font metrics
                font->GetMetrics(metrics);

                return true;
            }

            bool WinDisplay::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                DWRITE_FONT_METRICS fm;

                // Obtain the font family
                font_t *custom          = NULL;
                IDWriteFontFamily *ff   = get_font_family(f, NULL, &custom);
                lsp_finally{ safe_release(ff); };

                // Try to obtain font metrics, seek for custom settings first
                bool found = false;
                if (custom != NULL)
                    found = get_font_metrics(f, custom->family, &fm);
                if ((!found) && (ff != NULL))
                    found = get_font_metrics(f, ff, &fm);
                if (!found)
                    return false;

                float ratio     = f.size() / float(fm.designUnitsPerEm);
                fp->Ascent      = fm.ascent * ratio;
                fp->Descent     = fm.descent * ratio;
                fp->Height      = (fm.ascent + fm.descent + fm.lineGap) * ratio;

                return true;
            }

            bool WinDisplay::try_get_text_parameters(
                const Font &f,
                const WCHAR *fname,
                IDWriteFontCollection *fc,
                IDWriteFontFamily *ff,
                text_parameters_t *tp,
                const WCHAR *text,
                ssize_t length)
            {
                // Obtain font metrics
                DWRITE_FONT_METRICS fm;
                if (!get_font_metrics(f, ff, &fm))
                    return false;

                // Create text layout
                IDWriteTextLayout *tl   = create_text_layout(f, fname, fc, ff, text, length);
                if (tl == NULL)
                    return false;
                lsp_finally{ safe_release(tl); };

                // Get text layout metrics and font metrics
                DWRITE_TEXT_METRICS tm;
                tl->GetMetrics(&tm);

                float ratio     = f.size() / float(fm.designUnitsPerEm);
                tp->Width       = tm.width;
                tp->Height      = (fm.ascent + fm.descent + fm.lineGap) * ratio;
                tp->XAdvance    = tm.width;
                tp->YAdvance    = tp->Height;
                tp->XBearing    = (f.italic()) ? sinf(0.033f * M_PI) * tp->Height : 0.0f;
                tp->YBearing    = - fm.capHeight * ratio;

                return true;
            }

            bool WinDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                const WCHAR *pText = (text != NULL) ? reinterpret_cast<const WCHAR *>(text->get_utf16(first, last)) : NULL;
                if (pText == NULL)
                    return false;
                size_t range = text->range_length(first, last);

                // Obtain the font family
                LSPString family_name;
                font_t *custom          = NULL;
                IDWriteFontFamily *ff   = get_font_family(f, &family_name, &custom);
                lsp_finally{ safe_release(ff); };

                // Process custom font first
                bool found = false;
                if (custom != NULL)
                    found = try_get_text_parameters(f, custom->wname, custom->collection, custom->family, tp, pText, range);
                if ((!found) && (ff != NULL))
                    found = try_get_text_parameters(f, family_name.get_utf16(), NULL, ff, tp, pText, range);

                return found;
            }

            HCURSOR WinDisplay::translate_cursor(mouse_pointer_t pointer)
            {
                HCURSOR res = vCursors[pointer];
                if (res != NULL)
                    return res;

            #define TRC(mp, idc) \
                case mp: res = LoadCursor(NULL, idc); break;

                switch (pointer)
                {
                    // Special case: empty cursor
                    case MP_NONE:
                        res = CreateCursor(
                            GetModuleHandleW(NULL),     // hInst
                            0,                          // xHotSpot
                            0,                          // yHotSpot
                            1,                          // nWidth
                            1,                          // nHeight
                            none_cursor_and,            // pvANDPlane
                            none_cursor_xor             // pvXORPlane
                        );
                        break;

                    // Supported built-in cursors
                    TRC(MP_ARROW, IDC_ARROW)
                    TRC(MP_HAND, IDC_HAND)
                    TRC(MP_CROSS, IDC_CROSS)
                    TRC(MP_IBEAM, IDC_IBEAM)
                    TRC(MP_SIZE, IDC_SIZE)
                    TRC(MP_SIZE_NESW, IDC_SIZENESW)
                    TRC(MP_SIZE_NS, IDC_SIZENS)
                    TRC(MP_SIZE_WE, IDC_SIZEWE)
                    TRC(MP_SIZE_NWSE, IDC_SIZENWSE)
                    TRC(MP_UP_ARROW, IDC_UPARROW)
                    TRC(MP_HOURGLASS, IDC_WAIT)
                    TRC(MP_NO_DROP, IDC_NO)
                    TRC(MP_APP_START, IDC_APPSTARTING)
                    TRC(MP_HELP, IDC_HELP)

                    // Not matching icons
                    TRC(MP_ARROW_LEFT, IDC_UPARROW)
                    TRC(MP_ARROW_RIGHT, IDC_UPARROW)
                    TRC(MP_ARROW_UP, IDC_UPARROW)
                    TRC(MP_ARROW_DOWN, IDC_UPARROW)
                    TRC(MP_DRAG, IDC_ARROW)
                    TRC(MP_DRAW, IDC_ARROW)
                    TRC(MP_PLUS, IDC_CROSS)
                    TRC(MP_DANGER, IDC_NO)
                    TRC(MP_HSPLIT, IDC_NO)
                    TRC(MP_VSPLIT, IDC_NO)
                    TRC(MP_MULTIDRAG, IDC_NO)

                    default:
                        res     = LoadCursor(NULL, IDC_ARROW);
                        break;
                }
            #undef TRC

                return vCursors[pointer] = res;
            }

            status_t WinDisplay::grab_events(WinWindow *wnd, grab_t group)
            {
                // Check validity of argument
                if (group >= __GRAB_TOTAL)
                    return STATUS_BAD_ARGUMENTS;

                // Check that window does not belong to any active grab group
                size_t count = 0;
                for (size_t i=0; i<__GRAB_TOTAL; ++i)
                {
                    lltl::parray<WinWindow> &g = vGrab[i];
                    if (g.index_of(wnd) >= 0)
                    {
                        lsp_warn("Grab duplicated for window %p (id=%p)", wnd, wnd->hWindow);
                        return STATUS_DUPLICATED;
                    }
                    count   += g.size();
                }

                // Add a grab
                if (!vGrab[group].add(wnd))
                    return STATUS_NO_MEM;

                // Obtain a grab if necessary
                if (count > 0)
                    return STATUS_OK;

                lsp_trace("Setting grab for display=%p", this);
                return install_windows_hooks();
            }

            status_t WinDisplay::ungrab_events(WinWindow *wnd)
            {
                bool found = false;

                // Check that window does belong to any active grab group
                size_t count = 0;
                for (size_t i=0; i<__GRAB_TOTAL; ++i)
                {
                    lltl::parray<WinWindow> &g = vGrab[i];
                    if (g.premove(wnd))
                        found = true;
                    count      += g.size();
                }

                // Return error if not found
                if (!found)
                {
                    lsp_trace("No grab found for window %p (%lx)", wnd, long(wnd->hWindow));
                    return STATUS_NO_GRAB;
                }

                // Need to release X11 grab?
                if (count > 0)
                    return STATUS_OK;

                lsp_trace("Releasing grab for display=%p", this);
                return uninstall_windows_hooks();
            }

            status_t WinDisplay::install_windows_hooks()
            {
                lock_handlers(true);
                lsp_finally { unlock_handlers(); };

                // Create mouse hook
                if (hMouseHook == NULL)
                {
                    hMouseHook = SetWindowsHookExW(
                        WH_MOUSE_LL,
                        mouse_hook,
                        NULL,
                        0);
                    if (hMouseHook == NULL)
                    {
                        lsp_error("Error setting up mouse hook code=%ld", long(GetLastError()));
                        return STATUS_UNKNOWN_ERR;
                    }
                }

                // Create keyboard hook
                if (hKeyboardHook == NULL)
                {
                    hKeyboardHook = SetWindowsHookExW(
                        WH_KEYBOARD_LL,
                        keyboard_hook,
                        NULL,
                        0);
                    if (hKeyboardHook == NULL)
                    {
                        lsp_error("Error setting up keyboard hook code=%ld", long(GetLastError()));
                        return STATUS_UNKNOWN_ERR;
                    }
                }

                // Install self to the list
                for (WinDisplay *dpy = pHandlers; dpy != NULL; dpy = dpy->pNextHandler)
                    if (dpy == this)
                        return STATUS_OK;

                pNextHandler    = pHandlers;
                pHandlers       = this;

                return STATUS_OK;
            }

            status_t WinDisplay::uninstall_windows_hooks()
            {
                lock_handlers(true);
                lsp_finally { unlock_handlers(); };

                // Remove self from the list
                for (WinDisplay **dpy = &pHandlers; *dpy != NULL; dpy = &((*dpy)->pNextHandler))
                    if (*dpy == this)
                    {
                        *dpy = (*dpy)->pNextHandler;
                        break;
                    }

                if (pHandlers != NULL)
                    return STATUS_OK;

                // Kill hooks
                if (hMouseHook != NULL)
                {
                    UnhookWindowsHookEx(hMouseHook);
                    hMouseHook      = NULL;
                }
                if (hKeyboardHook != NULL)
                {
                    UnhookWindowsHookEx(hKeyboardHook);
                    hKeyboardHook   = NULL;
                }

                return STATUS_OK;
            }

            LRESULT CALLBACK WinDisplay::mouse_hook(int nCode, WPARAM wParam, LPARAM lParam)
            {
                lock_handlers(false);
                lsp_finally { unlock_handlers(); };

                LRESULT     res = CallNextHookEx(hMouseHook, nCode, wParam, lParam);

                if (nCode >= 0)
                {
                    // Form the list of handlers
                    lltl::parray<WinDisplay> handlers;
                    for (WinDisplay *dpy = pHandlers; dpy != NULL; dpy = dpy->pNextHandler)
                        handlers.add(dpy);

                    // Process the event
                    for (size_t i=0, n=handlers.size(); i<n; ++i)
                        handlers[i]->process_mouse_hook(nCode, wParam, lParam);
                }
                return res;
            }

            LRESULT CALLBACK WinDisplay::keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
            {
                lock_handlers(false);
                lsp_finally { unlock_handlers(); };

                LRESULT     res = CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);

                if (nCode >= 0)
                {
                    // Form the list of handlers
                    lltl::parray<WinDisplay> handlers;
                    for (WinDisplay *dpy = pHandlers; dpy != NULL; dpy = dpy->pNextHandler)
                        handlers.add(dpy);

                    // Process the event
                    for (size_t i=0, n=handlers.size(); i<n; ++i)
                        handlers[i]->process_keyboard_hook(nCode, wParam, lParam);
                }
                return res;
            }

            bool WinDisplay::fill_targets()
            {
                // Check if there is grab enabled and obtain list of receivers
                for (ssize_t i=__GRAB_TOTAL-1; i>=0; --i)
                {
                    lltl::parray<WinWindow> &g = vGrab[i];
                    if (g.size() <= 0)
                        continue;

                    // Add listeners from grabbing windows
                    for (size_t j=0; j<g.size();)
                    {
                        WinWindow *wnd = g.uget(j);
                        if (wnd == NULL)
                        {
                            g.remove(j);
                            continue;
                        }
                        sTargets.add(wnd);
                        ++j;
                    }

                    // Finally, break if there are target windows
                    if (sTargets.size() > 0)
                        return true;
                }

                return false;
            }

            bool WinDisplay::has_grabbing_events()
            {
                for (ssize_t i=__GRAB_TOTAL-1; i>=0; --i)
                {
                    lltl::parray<WinWindow> &g = vGrab[i];
                    if (g.size() > 0)
                        return true;
                }
                return false;
            }

            void WinDisplay::process_mouse_hook(int nCode, WPARAM wParam, LPARAM lParam)
            {
                /**
                 * @param nCode A code that the hook procedure uses to determine how to process the message.
                 *      If nCode is less than zero, the hook procedure must pass the message to the CallNextHookEx
                 *      function without further processing and should return the value returned by CallNextHookEx.
                 * @param wParam The identifier of the mouse message. This parameter can be one of the following messages:
                 *      WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_RBUTTONDOWN, or WM_RBUTTONUP
                 * @param lParam A pointer to a MSLLHOOKSTRUCT structure.
                 */
                // lsp_trace("mouse_hook");

                if (nCode != HC_ACTION)
                    return;

                // Preprocess the message
                BYTE kState[256];
                if (!GetKeyboardState(kState))
                    return;

                UINT uMsg   = wParam;
                const MSLLHOOKSTRUCT *mou = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
                wParam      = encode_mouse_keystate(kState);

                switch (uMsg)
                {
                    case WM_LBUTTONDOWN:
                    case WM_MBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_MBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_MOUSEMOVE:
                        break;

                    case WM_XBUTTONDOWN:
                    case WM_XBUTTONUP:
                        /**
                         * @param mouseData If the message is WM_XBUTTONDOWN, WM_XBUTTONUP, the high-order word specifies
                         * which X button was pressed or released, and the low-order word is reserved.
                         */
                        wParam |= (HIWORD(mou->mouseData)) << 16;
                        break;

                    case WM_MOUSEWHEEL:
                    case WM_MOUSEHWHEEL:
                        //! @param mouseData: If the message is WM_MOUSEWHEEL, the high-order word of this member is the wheel delta.
                        //! The low-order word is reserved. A positive value indicates that the wheel was rotated forward, away
                        //! from the user; a negative value indicates that the wheel was rotated backward, toward the user. One
                        //! wheel click is defined as WHEEL_DELTA, which is 120.
                        wParam |= (HIWORD(mou->mouseData)) << 16;
                        break;
                    default:
                        return;
                }

                // Form list of targets
                lsp_finally { sTargets.clear(); };
                if (!fill_targets())
                    return;

                // Obtain mouse capture, do not deliver events
                HWND capture = GetCapture();

                // Notify all targets
                for (size_t i=0, n=sTargets.size(); i<n; ++i)
                {
                    WinWindow *wnd = sTargets.uget(i);
                    if ((wnd == NULL) || (wnd->hWindow == NULL))
                        continue;

                    // Do not deliver messages to the window which is currently capturing data
                    if (wnd->hWindow == capture)
                        continue;

                    /**
                     * @param pt The x- and y-coordinates of the cursor, in per-monitor-aware screen coordinates.
                     */
                    POINT pt    = mou->pt;
                    RECT rect;
                    if (!ScreenToClient(wnd->hWindow, &pt))
                        continue;
                    if (!GetClientRect(wnd->hWindow, &rect))
                        continue;

                    // Skip events that are inside of client area of the window. Process them as usual.
                    if ((pt.x >= rect.left) && (pt.x <= rect.right) &&
                        (pt.y >= rect.top) && (pt.y <= rect.bottom))
                        continue;

                    lParam      = MAKELONG(pt.x, pt.y);
                    SendMessageW(wnd->hWindow, uMsg, wParam, lParam);
//                    wnd->process_event(uMsg, wParam, lParam, mou->time, true);
                }
            }

            void WinDisplay::process_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
            {
                // lsp_trace("keyboard_hook");

                /**
                 * @param nCode A code that the hook procedure uses to determine how to process the message.
                 *      If nCode is less than zero, the hook procedure must pass the message to the CallNextHookEx
                 *      function without further processing and should return the value returned by CallNextHookEx.
                 * @param wParam The identifier of the keyboard message. This parameter can be one of the following
                 *      messages: WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP.
                 * @param lParam A pointer to a KBDLLHOOKSTRUCT structure.
                 */
                if (nCode != HC_ACTION)
                    return;

                // Preprocess parameters
                UINT uMsg = wParam;
                const KBDLLHOOKSTRUCT *kbd = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

                switch (uMsg)
                {
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                        break;
                    default:
                        return;
                }

                wParam  = kbd->vkCode;
                lParam  = 1; // Repeat count
                lParam |= ((kbd->scanCode & 0xff) << 16); // scan code
                lParam |= (kbd->flags & LLKHF_EXTENDED) ? (1L << 24) : 0; // the extended key flag
                lParam |= (kbd->flags & LLKHF_ALTDOWN) ? (1L << 29) : 0; // the context code
                lParam |= (kbd->flags & LLKHF_UP) ? (1L << 31) : 0; // the transition state

                // Form list of targets
                lsp_finally { sTargets.clear(); };
                if (!fill_targets())
                    return;

                // Notify all targets
                for (size_t i=0, n=sTargets.size(); i<n; ++i)
                {
                    WinWindow *wnd = sTargets.uget(i);
                    SendMessageW(wnd->hWindow, uMsg, wParam, lParam);
//                    if (wnd != NULL)
//                        wnd->process_event(uMsg, wParam, lParam, kbd->time, true);
                }
            }

            void WinDisplay::lock_handlers(bool preempt)
            {
                // Check that we are already owner of the lock
                DWORD thread_id = GetCurrentThreadId();
                if (nThreadId != thread_id)
                {
                    // We're not owner of the lock.
                    // Perform the first lock when it will be available (zero)
                    while (!atomic_cas(&hLock, 0, 1)) {
                        SwitchToThread();
                    }

                    // Lock has been acquired, store us as the new owner
                    nThreadId   = thread_id;
                    return;
                }

                // We are the owners of the lock.
                // Just increment the counter.
                while (true)
                {
                    atomic_t count = hLock;
                    if (atomic_cas(&hLock, count, count + 1))
                        break;
                }
            }

            void WinDisplay::unlock_handlers()
            {
                // Ensure that our thread is an exclusive owner of the lock
                DWORD thread_id = GetCurrentThreadId();
                if (nThreadId != thread_id)
                    return;

                // Decrement the counter
                // We are already owner, so we can reset thread identifier
                // before decrementing the counter
                while (true)
                {
                    atomic_t count = hLock;
                    if (count == 1)
                        nThreadId   = 0;
                    if (atomic_cas(&hLock, count, count - 1))
                        return;
                }
            }

            bool WinDisplay::is_hookable_event(UINT uMsg)
            {
                switch (uMsg)
                {
                    // Mouse events
                    case WM_LBUTTONDOWN:
                    case WM_MBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_MBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_MOUSEMOVE:
                    case WM_XBUTTONDOWN:
                    case WM_XBUTTONUP:
                    case WM_MOUSEWHEEL:
                    case WM_MOUSEHWHEEL:

                    // Keyboard events
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                        return true;

                    default:
                        break;
                }
                return false;
            }

            bool WinDisplay::has_mime_types(const char * const * src_list, const char * const * check)
            {
                for( ; (src_list != NULL) && (*src_list != NULL); ++src_list)
                    if (index_of_mime(*src_list, check) >= 0)
                        return true;

                return false;
            }

            LRESULT CALLBACK WinDisplay::clipboard_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                WinDisplay *dpy = reinterpret_cast<WinDisplay *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

                switch (uMsg)
                {
                    case WM_CREATE:
                    {
                        CREATESTRUCTW *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                        dpy = reinterpret_cast<WinDisplay *>(create->lpCreateParams);
                        if (dpy != NULL)
                            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dpy));
                        break;
                    }
                    case WM_RENDERFORMAT:
                    {
                        // wParam - The clipboard format to be rendered.
                        lsp_trace("WM_RENDERFORMAT fmt=%d", int(wParam));
                        dpy->render_clipboard_format(wParam);
                        break;
                    }
                    case WM_RENDERALLFORMATS:
                    {
                        lsp_trace("WM_RENDERALLFORMATS");
                        if (!OpenClipboard(dpy->hClipWnd))
                            return 0;
                        lsp_finally{ CloseClipboard(); };
                        if (GetClipboardOwner() != dpy->hClipWnd)
                            return 0;

                        // Render each format
                        UINT fmt = 0;
                        while (true)
                        {
                            fmt = EnumClipboardFormats(fmt);
                            if (fmt == 0)
                                break;
                            dpy->render_clipboard_format(fmt);
                        }

                        break;
                    }
                    case WM_DESTROYCLIPBOARD:
                    {
                        dpy->destroy_clipboard();
                        return 0;
                    }
                    case WM_USER:
                    {
                        ws::timestamp_t ts  = system::get_time_millis();
                        dpy->process_pending_tasks(ts);
                        return 0;
                    }
                    default:
                        break;
                }

                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }

            void WinDisplay::destroy_clipboard()
            {
                // Release clipboard data
                if (pClipData != NULL)
                {
                    pClipData->release();
                    pClipData = NULL;
                }

                // Free previously allocated clipboard memory
                for (size_t i=0, n=vClipMemory.size(); i<n; ++i)
                {
                    HGLOBAL data = vClipMemory.uget(i);
                    if (data != NULL)
                        GlobalFree(data);
                }
                vClipMemory.clear();
            }

            void *WinDisplay::clipboard_global_alloc(const void *src, size_t bytes)
            {
                if (src == NULL)
                    return NULL;

                // Allocate the memory region and copy data
                HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (g == NULL)
                    return NULL;

                void *ptr = GlobalLock(g);
                if (ptr == NULL)
                {
                    GlobalFree(g);
                    return NULL;
                }

                memcpy(ptr, src, bytes);
                GlobalUnlock(g);

                // Register the memory region
                if (vClipMemory.add(g))
                    return g;

                // Something went wrong
                GlobalFree(g);
                return NULL;
            }

            status_t WinDisplay::set_clipboard(size_t id, IDataSource *ds)
            {
                // Check arguments
                if ((id < 0) || (id >= _CBUF_TOTAL))
                    return STATUS_BAD_ARGUMENTS;
                // Ignore data for any clipboard except the CBUF_CLIPBOARD
                // This will prevent us from replacing the Ctrl-C/Ctrl-V clipboard data
                // by the text selection data
                if (id != CBUF_CLIPBOARD)
                {
                    ds->acquire();
                    ds->release();
                    return STATUS_OK;
                }

                // Release previous clipboard handler
                if (pClipData != NULL)
                {
                    pClipData->release();
                    pClipData = NULL;
                }

                // Work with the clipboard
                if (!OpenClipboard(hClipWnd))
                    return STATUS_UNKNOWN_ERR;
                lsp_finally{ CloseClipboard(); };
                EmptyClipboard();

                // If there is no replacement - just return
                if (ds == NULL)
                    return STATUS_OK;

                // Save pointer to clipboard data
                ds->acquire();
                pClipData = ds;

                // Now we need to register different clipboard formats
                const char * const * mimes = ds->mime_types();
                if (has_mime_types(mimes, unicode_mimes))
                    SetClipboardData(CF_UNICODETEXT, NULL);
                if (has_mime_types(mimes, oem_mimes))
                    SetClipboardData(CF_OEMTEXT, NULL);
                if (has_mime_types(mimes, ansi_mimes))
                    SetClipboardData(CF_TEXT, NULL);

                // Register custom clipboard formats
                LSPString tmp;
                for( ; (mimes != NULL) && (*mimes != NULL); ++mimes)
                {
                    if (!tmp.set_utf8(*mimes))
                        continue;
                    const WCHAR *name = tmp.get_utf16();
                    if (name == NULL)
                        continue;

                    UINT fmt = RegisterClipboardFormatW(name);
                    if (fmt != 0)
                        SetClipboardData(fmt, NULL);
                }

                return STATUS_OK;
            }

            void WinDisplay::render_clipboard_format(UINT fmt)
            {
                if (pClipData == NULL)
                    return;
                pClipData->acquire();
                lsp_finally { pClipData->release(); };

                void *ptr   = NULL;
                if (fmt == CF_UNICODETEXT)
                    ptr         = make_clipboard_utf16_text();
                else if (fmt == CF_OEMTEXT)
                    ptr         = make_clipboard_native_text();
                else if (fmt == CF_TEXT)
                    ptr         = make_clipboard_ascii_text();
                else
                {
                    LSPString fmt_name;
                    if (read_clipboard_format_name(fmt, &fmt_name) == STATUS_OK)
                        ptr         = make_clipboard_custom_format(fmt_name.get_utf8());
                }

                if (ptr != NULL)
                    SetClipboardData(fmt, ptr);
            }

            wssize_t WinDisplay::read_clipboard_blob(io::IOutStream *os, const char *format)
            {
                io::IInStream *is = pClipData->open(format);
                if (is == NULL)
                    return -STATUS_NOT_SUPPORTED;
                lsp_finally{
                    is->close();
                    delete is;
                };
                return is->sink(os);
            }

            void *WinDisplay::make_clipboard_utf16_text()
            {
                io::OutMemoryStream os;
                LSPString tmp;

                for (size_t index = 0; unicode_mimes[index] != NULL; ++index)
                {
                    os.clear();
                    wssize_t res = read_clipboard_blob(&os, unicode_mimes[index]);
                    if (res < 0)
                        continue;

                    bool ok = false;
                    switch (index)
                    {
                        case 0:
                        case 1:
                            ok = tmp.set_utf8(reinterpret_cast<const char *>(os.data()), size_t(res));
                            break;
                        case 2: // UTF-16LE
                            ok = __IF_LEBE(
                                tmp.set_utf16(reinterpret_cast<const lsp_utf16_t *>(os.data()), size_t(res / sizeof(lsp_utf16_t))),
                                tmp.set_native(reinterpret_cast<const char *>(os.data()), size_t(res), "UTF16-LE")
                            );
                            break;
                        case 3: // UTF-16BE
                            ok = __IF_LEBE(
                                tmp.set_native(reinterpret_cast<const char *>(os.data()), size_t(res), "UTF16-BE"),
                                tmp.set_utf16(reinterpret_cast<const lsp_utf16_t *>(os.data()), size_t(res / sizeof(lsp_utf16_t)))
                            );
                            break;
                        default:
                            break;
                    }
                    if (!ok)
                        continue;
                    if (!tmp.to_dos())
                        return NULL;

                    const lsp_utf16_t *data = tmp.get_utf16();
                    return clipboard_global_alloc(data, tmp.temporal_size());
                }

                return NULL;
            }

            void *WinDisplay::make_clipboard_native_text()
            {
                io::OutMemoryStream os;
                wssize_t res = read_clipboard_blob(&os, oem_mimes[0]);
                if (res < 0)
                    return NULL;

                LSPString tmp;
                if (!tmp.set_native(reinterpret_cast<const char *>(os.data()), size_t(res)))
                    return NULL;
                if (!tmp.to_dos())
                    return NULL;

                const char *data = tmp.get_native();
                return clipboard_global_alloc(data, tmp.temporal_size());
            }

            void *WinDisplay::make_clipboard_ascii_text()
            {
                io::OutMemoryStream os;
                wssize_t res = read_clipboard_blob(&os, ansi_mimes[0]);
                if (res < 0)
                    return NULL;

                LSPString tmp;
                if (!tmp.set_ascii(reinterpret_cast<const char *>(os.data()), size_t(res)))
                    return NULL;
                if (!tmp.to_dos())
                    return NULL;

                const char *data = tmp.get_ascii();
                return clipboard_global_alloc(data, tmp.temporal_size());
            }

            void *WinDisplay::make_clipboard_custom_format(const char *name)
            {
                io::OutMemoryStream os;
                wssize_t res = read_clipboard_blob(&os, name);
                return (res >= 0) ? clipboard_global_alloc(os.data(), res) : NULL;
            }

            size_t WinDisplay::append_mimes(lltl::parray<char> *list, const char * const * mimes)
            {
                size_t added = 0;
                for ( ; (mimes != NULL) && (*mimes != NULL); ++mimes)
                    if (list->add(const_cast<char *>(*mimes)))
                        ++added;
                return added;
            }

            status_t WinDisplay::get_clipboard(size_t id, IDataSink *dst)
            {
                typedef struct range_t
                {
                    ssize_t first;
                    ssize_t count;
                } range_t;

                typedef struct fmt_t
                {
                    size_t idx;
                    LSPString name;
                } fmt_t;

                dst->acquire();
                lsp_finally { dst->release(); };

                // Open clipboard
                if (!OpenClipboard(hClipWnd))
                    return STATUS_UNKNOWN_ERR;
                lsp_finally{ CloseClipboard(); };

                // Enumerate all available formats
                lltl::parray<char> mimes;
                lltl::parray<fmt_t> formats;
                lsp_finally {
                    for (size_t i=0, n=formats.size(); i<n; ++i)
                    {
                        fmt_t *fmt = formats.uget(i);
                        delete fmt;
                    }
                };

                UINT fmt            = 0;
                ssize_t idx         = 0;
                range_t unicode     = { 0, 0 };
                range_t oem         = { 0, 0 };
                range_t ansi        = { 0, 0 };

                while (true)
                {
                    fmt = EnumClipboardFormats(fmt);
                    if (fmt == 0)
                        break;

                    if (fmt == CF_UNICODETEXT)
                    {
                        unicode.first   = idx;
                        unicode.count   = append_mimes(&mimes, unicode_mimes);
                        idx            += lsp_max(unicode.count, 0);
                    }
                    else if (fmt == CF_OEMTEXT)
                    {
                        oem.first       = idx;
                        oem.count       = append_mimes(&mimes, oem_mimes);
                        idx            += lsp_max(oem.count, 0);
                    }
                    else if (fmt == CF_TEXT)
                    {
                        ansi.first      = idx;
                        ansi.count      = append_mimes(&mimes, ansi_mimes);
                        idx            += lsp_max(ansi.count, 0);
                    }
                    else
                    {
                        // Append custom format
                        fmt_t *f        = new fmt_t;
                        f->idx          = idx;

                        lsp_finally{
                            if (f != NULL)
                                delete f;
                        };
                        if (f == NULL)
                            continue;
                        if (read_clipboard_format_name(fmt, &f->name) != STATUS_OK)
                            continue;
                        const char *utf = f->name.get_utf8();
                        if (utf == NULL)
                            continue;
                        if (!mimes.add(const_cast<char *>(utf)))
                            continue;
                        if (formats.add(f))
                        {
                            ++idx;
                            f           = NULL;
                        }
                    }
                }

                // Append terminator
                if (!mimes.add(static_cast<char *>(NULL)))
                    return STATUS_NO_MEM;

                // Now open the sink
                ssize_t fmt_id  = dst->open(mimes.array());
                if ((fmt_id < 0) || (fmt_id >= ssize_t(mimes.size() - 1)))
                    return -fmt_id;

                // Determine what format has been selected
                if ((fmt_id >= unicode.first) && (fmt_id < (unicode.first + unicode.count)))
                    fmt         = CF_UNICODETEXT;
                else if ((fmt_id >= oem.first) && (fmt_id < (oem.first + oem.count)))
                    fmt         = CF_OEMTEXT;
                else if ((fmt_id >= ansi.first) && (fmt_id < (ansi.first + ansi.count)))
                    fmt         = CF_TEXT;

                HGLOBAL g       = reinterpret_cast<HGLOBAL>(GetClipboardData(fmt));
                if (g == NULL)
                    return dst->close(STATUS_NO_MEM);

                return sink_hglobal_contents(dst, g, fmt, mimes.uget(fmt_id));
            }

            WinWindow *WinDisplay::set_drag_window(WinWindow *wnd)
            {
                WinWindow *old  = pDragWindow;
                pDragWindow     = wnd;
                return old;
            }

            bool WinDisplay::unset_drag_window(WinWindow *wnd)
            {
                if (pDragWindow != wnd)
                    return false;
                pDragWindow     = NULL;
                return true;
            }

            WinWindow *WinDisplay::drag_window()
            {
                return pDragWindow;
            }

            bool WinDisplay::r3d_backend_supported(const r3d::backend_metadata_t *meta)
            {
                // WinDisplay display supports Windows window handles
                if (meta->wnd_type == r3d::WND_HANDLE_WINDOWS)
                    return true;
                return IDisplay::r3d_backend_supported(meta);
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


