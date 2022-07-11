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
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/runtime/system.h>

#include <private/win/WinDisplay.h>
#include <private/win/WinWindow.h>

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
            const WCHAR *WinDisplay::WINDOW_CLASS_NAME = L"lsp-ws-lib window";

            static const BYTE none_cursor_and[] = { 0xff };
            static const BYTE none_cursor_xor[] = { 0 };

            template <class T>
                static inline void safe_release(T * &obj)
                {
                    if (obj != NULL)
                    {
                        obj->Release();
                        obj = NULL;
                    }
                }

            WinDisplay::WinDisplay():
                IDisplay()
            {
                bExit           = false;
                pD2D1Factroy    = NULL;
                pWICFactory     = NULL;
                pDWriteFactory  = NULL;

                bzero(&sPendingMessage, sizeof(sPendingMessage));
                sPendingMessage.message     = WM_NULL;

                for (size_t i=0; i<__MP_COUNT; ++i)
                    vCursors[i]     = NULL;
            }

            WinDisplay::~WinDisplay()
            {
            }

            status_t WinDisplay::init(int argc, const char **argv)
            {
                HRESULT hr;

                hr = CoInitialize(NULL);
                if (FAILED(hr))
                    return STATUS_UNKNOWN_ERR;

                bExit           = false;

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

                return STATUS_OK;
            }

            void WinDisplay::destroy()
            {
                UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(NULL));

                // Destroy cursors
                if (vCursors[MP_NONE] != NULL)
                    DestroyCursor(vCursors[MP_NONE]);
                for (size_t i=0; i<__MP_COUNT; ++i)
                    vCursors[i]     = NULL;

                // Destroy monitors
                drop_monitors(&vMonitors);
                drop_font_cache(&vFontCache);

                // Release factoreis
                safe_release( pDWriteFactory );
                safe_release( pWICFactory );
                safe_release( pD2D1Factroy );
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

//                lsp_trace("Received event: this=%p, hwnd=%p, uMsg=%d(0x%x), wParam=%d(0x%x), lParam=%p",
//                    _this, hwnd, int(uMsg), int(uMsg), int(wParam), int(wParam), lParam);

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

            bool WinDisplay::create_font_cache()
            {
                // Get font collection
                IDWriteFontCollection *fc = NULL;
                HRESULT hr      = pDWriteFactory->GetSystemFontCollection(&fc, FALSE);
                if (FAILED(hr))
                    return false;
                lsp_finally( safe_release(fc); );

                // Temporary variables to store font family names
                WCHAR *buf  = NULL;
                size_t blen = 0;
                lsp_finally(
                    if (buf != NULL)
                        free(buf);
                );
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
                    lsp_finally( safe_release(ff); );

                    // Obtain names of the font family
                    IDWriteLocalizedStrings *fnames = NULL;
                    hr = ff->GetFamilyNames(&fnames);
                    if ((FAILED(hr)) || (fnames == NULL))
                        continue;
                    lsp_finally( safe_release(fnames); );

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
                        lsp_finally( DeleteObject(hfont); );

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

            IDWriteFontFamily *WinDisplay::get_font_family(const Font &f, LSPString *name)
            {
                // Prepare font name
                LSPString tmp;
                if (!tmp.set_utf8(f.name()))
                    return NULL;
                tmp.tolower();

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
                const LSPString *fname,
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
                    fname->get_utf16(),         // Font family name
                    NULL,                       // Font collection (NULL sets it to use the system font collection)
                    (f.bold())   ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
                    (f.italic()) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    f.size(),
                    wLocale,
                    &tf);
                if ((FAILED(hr)) || (tf == NULL))
                    return NULL;
                lsp_finally( safe_release(tf); );

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
                    return false;
                lsp_finally( safe_release(font); );

                // Get font metrics
                font->GetMetrics(metrics);

                return true;
            }

            bool WinDisplay::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // Obtain the font family
                IDWriteFontFamily *ff   = get_font_family(f, NULL);
                if (ff == NULL)
                    return false;
                lsp_finally( safe_release(ff); );

                // Get font metrics
                DWRITE_FONT_METRICS fm;
                if (!get_font_metrics(f, ff, &fm))
                    return false;

                float ratio     = f.size() / float(fm.designUnitsPerEm);
                fp->Ascent      = fm.ascent * ratio;
                fp->Descent     = fm.descent * ratio;
                fp->Height      = (fm.ascent + fm.descent + fm.lineGap) * ratio;

                return true;
            }

            bool WinDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                const WCHAR *pText = (text != NULL) ? reinterpret_cast<const WCHAR *>(text->get_utf16(first, last)) : NULL;
                if (pText == NULL)
                    return false;

                // Obtain the font family
                LSPString family_name;
                IDWriteFontFamily *ff   = get_font_family(f, &family_name);
                if (ff == NULL)
                    return false;
                lsp_finally( safe_release(ff); );

                // Obtain font metrics
                DWRITE_FONT_METRICS fm;
                if (!get_font_metrics(f, ff, &fm))
                    return false;

                // Create text layout
                IDWriteTextLayout *tl   = create_text_layout(f, &family_name, ff, pText, text->range_length(first, last));
                if (tl == NULL)
                    return false;
                lsp_finally( safe_release(tl); );

                // Get text layout metrics and font metrics
                DWRITE_TEXT_METRICS tm;
                tl->GetMetrics(&tm);

                float ratio     = f.size() / float(fm.designUnitsPerEm);
                tp->XBearing    = 0.0f;
                tp->YBearing    = - fm.capHeight * ratio;
                tp->Width       = tm.width;
                tp->Height      = tm.height;
                tp->XAdvance    = tm.width;
                tp->YAdvance    = (fm.ascent + fm.descent + fm.lineGap) * ratio;

                return true;
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
                    TRC(MP_PLUS, IDC_ARROW)
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

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


