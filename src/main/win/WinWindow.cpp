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

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/ws/win/decode.h>

#include <private/win/WinDisplay.h>
#include <private/win/WinDDSurface.h>
#include <private/win/WinWindow.h>

#include <windows.h>
#include <windowsx.h>

#ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL 0x020e
#endif /* WM_MOUSEHWHEEL */

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            WinWindow::WinWindow(WinDisplay *dpy, HWND wnd, IEventHandler *handler, bool wrapper):
                ws::IWindow(dpy, handler)
            {
                pDisplay                = dpy;
                if (wrapper)
                {
                    hWindow                 = wnd;
                    hParent                 = INVALID_HWND;
                }
                else
                {
                    hWindow                 = INVALID_HWND;
                    hParent                 = wnd;
                }
                pSurface                = NULL;
                pOldUserData            = reinterpret_cast<LONG_PTR>(reinterpret_cast<void *>(NULL));
                pOldProc                = reinterpret_cast<WNDPROC>(NULL);
                bWrapper                = wrapper;
                bMouseInside            = false;
                nMouseCapture           = 0;

                sSize.nLeft             = 0;
                sSize.nTop              = 0;
                sSize.nWidth            = 32;
                sSize.nHeight           = 32;

                sConstraints.nMinWidth  = -1;
                sConstraints.nMinHeight = -1;
                sConstraints.nMaxWidth  = -1;
                sConstraints.nMaxHeight = -1;
                sConstraints.nPreWidth  = -1;
                sConstraints.nPreHeight = -1;

                enPointer               = MP_DEFAULT;
                enBorderStyle           = BS_SIZEABLE;
                nActions                = WA_ALL;

                bzero(&sSavedCursor, sizeof(sSavedCursor));
            }

            WinWindow::~WinWindow()
            {
                pDisplay        = NULL;
            }

            status_t WinWindow::init()
            {
                // Register the window
                if (pDisplay == NULL)
                    return STATUS_BAD_STATE;

                if (!bWrapper)
                {
                    // Create new window
                    hWindow         = CreateWindowExW(
                        0,                              // dwExStyle
                        WinDisplay::WINDOW_CLASS_NAME,  // lpClassName
                        L"",                            // lpWindowName
                        WS_OVERLAPPEDWINDOW,            // dwStyle
                        sSize.nLeft,                    // X
                        sSize.nTop,                     // Y
                        sSize.nWidth,                   // nWidth
                        sSize.nHeight,                  // nHeight
                        hParent,                        // hWndParent
                        NULL,                           // hMenu
                        GetModuleHandleW(NULL),         // hInstance
                        this                            // lpCreateParam
                    );

                    if (hWindow == NULL)
                    {
                        lsp_error("Error creating window: %ld", long(GetLastError()));
                        return STATUS_UNKNOWN_ERR;
                    }

                    commit_border_style(enBorderStyle, nActions);
                }
                else
                {
                    // Update pointer to the window routine
                    pOldProc        = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc)));
                    pOldUserData    = SetWindowLongPtrW(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                }

                // Create surface
                pSurface        = new WinDDSurface(pDisplay, hWindow, sSize.nWidth, sSize.nHeight);
                if (pSurface == NULL)
                    return STATUS_NO_MEM;

                // Enable all keyboard and mouse input for the window
                EnableWindow(hWindow, TRUE);

                return STATUS_OK;
            }

            void WinWindow::destroy()
            {
                if (hWindow == NULL)
                    return;

                SetWindowLongPtrW(hWindow, GWLP_USERDATA, pOldUserData);
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc));

                if (!bWrapper)
                    DestroyWindow(hWindow);

                hWindow     = NULL;
                pDisplay    = NULL;
            }

            void WinWindow::generate_enter_event(timestamp_t ts, const ws::event_t *ev)
            {
                if (bMouseInside)
                    return;

                ws::event_t xev     = *ev;
                xev.nCode           = UIE_MOUSE_IN;
                bMouseInside        = true;

                // Set mouse tracking
                TRACKMOUSEEVENT track;
                track.cbSize        = sizeof(track);
                track.dwFlags       = TME_LEAVE;
                track.hwndTrack     = hWindow;
                track.dwHoverTime   = 0;
                TrackMouseEvent(&track);

                // Update cursor to the current value and save the previous one
                sSavedCursor.cbSize = sizeof(sSavedCursor);
                if (GetCursorInfo(&sSavedCursor))
                {
                    HCURSOR cursor = pDisplay->translate_cursor(enPointer);
                    if (cursor != NULL)
                        SetCursor(cursor);

                    POINT coord = sSavedCursor.ptScreenPos;
                    if (ScreenToClient(hWindow, &coord))
                    {
                        xev.nLeft               = coord.x;
                        xev.nTop                = coord.y;
                    }
                    else
                    {
                        xev.nLeft               = 0;
                        xev.nTop                = 0;
                    }
                }
                else
                {
                    sSavedCursor.cbSize         = 0;
                    xev.nLeft                   = 0;
                    xev.nTop                    = 0;
                }

                // Notify the handler about mouse enter event
                handle_event(&xev);
            }

            LRESULT WinWindow::process_event(UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                // Translate the event and pass to handler
                event_t ue;
                init_event(&ue);

                system::time_t ts;
                system::get_time(&ts);
                timestamp_t xts = (timestamp_t(ts.seconds) * 1000) + (ts.nanos / 1000000);
                ue.nTime = xts;

                // Translate the event
                switch (uMsg)
                {
                    // Obtaining size constraints
                    case WM_GETMINMAXINFO:
                    {
                        // Contains information about a window's maximized size and position and
                        // it's minimum and maximum tracking size.
                        MINMAXINFO *info        = reinterpret_cast<MINMAXINFO *>(lParam);

                        size_limit_t sl         = sConstraints;
                        if (has_border())
                        {
                            // Get extra padding
                            ssize_t hborder         = GetSystemMetrics(SM_CXSIZEFRAME);
                            ssize_t vborder         = GetSystemMetrics(SM_CYSIZEFRAME);
                            ssize_t vcaption        = GetSystemMetrics(SM_CYCAPTION);

                            // Add padding to constraints
                            // TODO: handle windows with NONE border style
                            if (sl.nMinWidth >= 0)
                                sl.nMinWidth           += hborder * 2;
                            if (sl.nMaxWidth >= 0)
                                sl.nMaxWidth           += hborder * 2;
                            if (sl.nMinHeight >= 0)
                                sl.nMinHeight          += vcaption + vborder * 2;
                            if (sl.nMaxHeight >= 0)
                                sl.nMaxHeight          += vcaption + vborder * 2;
                        }

                        // Compute window constraints regarding to the computed contraints
                        info->ptMinTrackSize.x  = lsp_max(sl.nMinWidth, 1);
                        info->ptMinTrackSize.y  = lsp_max(sl.nMinHeight, 1);

                        if (sl.nMaxWidth >= 0)
                        {
                            ssize_t max_size        = lsp_max(sl.nMaxWidth, info->ptMinTrackSize.x);
                            info->ptMaxSize.x       = max_size;
                            info->ptMaxTrackSize.x  = max_size;
                        }
                        else
                        {
                            info->ptMaxSize.x       = lsp_max(info->ptMaxSize.x, info->ptMinTrackSize.x);
                            info->ptMaxTrackSize.x  = lsp_max(info->ptMaxTrackSize.x, info->ptMinTrackSize.x);
                        }

                        if (sl.nMaxHeight >= 0)
                        {
                            ssize_t max_size        = lsp_max(sl.nMaxHeight, info->ptMinTrackSize.y);
                            info->ptMaxSize.y       = max_size;
                            info->ptMaxTrackSize.y  = max_size;
                        }
                        else
                        {
                            info->ptMaxSize.y       = lsp_max(info->ptMaxSize.y, info->ptMinTrackSize.y);
                            info->ptMaxTrackSize.y  = lsp_max(info->ptMaxTrackSize.y, info->ptMinTrackSize.y);
                        }

                        // The message has been properly processed
                        return 0;
                    }

                    // Sizing, moving, showing
                    case WM_SIZE:
                    {
                        ue.nType                = UIE_RESIZE;
                        ue.nLeft                = sSize.nLeft;
                        ue.nTop                 = sSize.nTop;
                        ue.nWidth               = LOWORD(lParam); // The low-order word of lParam specifies the new width of the client area.
                        ue.nHeight              = HIWORD(lParam); // The high-order word of lParam specifies the new height of the client area.

                        handle_event(&ue);

                        InvalidateRect(hWindow, NULL, FALSE);
                        return 0;
                    }
                    case WM_MOVE:
                    {
                        ue.nType                = UIE_RESIZE;
                        ue.nLeft                = LOWORD(lParam);
                        ue.nTop                 = HIWORD(lParam);
                        ue.nWidth               = sSize.nWidth;
                        ue.nHeight              = sSize.nHeight;

                        handle_event(&ue);
                        return 0;
                    }
                    case WM_SHOWWINDOW:
                    {
                        ue.nType                = (wParam == TRUE) ? UIE_SHOW : UIE_HIDE;
                        bMouseInside            = false;
                        handle_event(&ue);
                        return 0;
                    }

                    // Closing the window
                    case WM_CLOSE:
                    {
                        ue.nType                = UIE_CLOSE;
                        handle_event(&ue);
                        return 0;
                    }

                    // Painting the window
                    case WM_PAINT:
                    {
                        PAINTSTRUCT ps;
                        BeginPaint(hWindow, &ps);

                        ue.nType                = UIE_REDRAW;
                        ue.nLeft                = ps.rcPaint.left;
                        ue.nTop                 = ps.rcPaint.top;
                        ue.nWidth               = ps.rcPaint.right - ps.rcPaint.left;
                        ue.nHeight              = ps.rcPaint.bottom - ps.rcPaint.top;

                        handle_event(&ue);

                        EndPaint(hWindow, &ps);

                        return 0;
                    }

                    // Mouse events
                    case WM_CAPTURECHANGED:
                    {
                        nMouseCapture           = 0;
                        ReleaseCapture();
                        return 0;
                    }
                    case WM_MOUSEMOVE:
                    {
                        ue.nType                = UIE_MOUSE_MOVE;
                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);
                        ue.nState               = decode_mouse_keystate(wParam);

                        generate_enter_event(xts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_LBUTTONDOWN:
                    case WM_RBUTTONDOWN:
                    case WM_MBUTTONDOWN:
                    {
                        ue.nType                = UIE_MOUSE_DOWN;
                        ue.nCode                = (uMsg == WM_LBUTTONDOWN) ? MCB_LEFT :
                                                  (uMsg == WM_RBUTTONDOWN) ? MCB_RIGHT :
                                                  MCB_MIDDLE;
                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);
                        ue.nState               = decode_mouse_keystate(wParam);

                        // Set mouse capture
                        if (!nMouseCapture)
                            SetCapture(hWindow);
                        nMouseCapture          |= 1 << ue.nCode;

                        lsp_trace("button down: %d", int(ue.nCode));
                        generate_enter_event(xts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_LBUTTONUP:
                    case WM_RBUTTONUP:
                    case WM_MBUTTONUP:
                    {
                        ue.nType                = UIE_MOUSE_UP;
                        ue.nCode                = (uMsg == WM_LBUTTONUP) ? MCB_LEFT :
                                                  (uMsg == WM_RBUTTONUP) ? MCB_RIGHT :
                                                  MCB_MIDDLE;
                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);
                        ue.nState               = decode_mouse_keystate(wParam);

                        nMouseCapture          &= ~(1 << ue.nCode);
                        if (nMouseCapture == 0)
                            ReleaseCapture();

                        lsp_trace("button up: %d", int(ue.nCode));
                        generate_enter_event(xts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_MOUSEWHEEL:
                    case WM_MOUSEHWHEEL:
                    {
                        ssize_t delta           = GET_WHEEL_DELTA_WPARAM(wParam);

                        ue.nType                = UIE_MOUSE_SCROLL;
                        if (uMsg == WM_MOUSEHWHEEL)
                            ue.nCode                = (delta > 0) ? MCD_RIGHT : MCD_LEFT;
                        else // uMsg == WM_MOUSEWHEEL
                            ue.nCode                = (delta > 0) ? MCD_UP : MCD_DOWN;

                        ue.nState               = decode_mouse_keystate(GET_KEYSTATE_WPARAM(wParam));
                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);

                        generate_enter_event(xts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_MOUSELEAVE:
                    {
                        bMouseInside            = false;

                        // Restore the previous cursor if it was saved
                        if (sSavedCursor.cbSize == sizeof(sSavedCursor))
                        {
                            HCURSOR cursor = pDisplay->translate_cursor(enPointer);
                            if (cursor != NULL)
                                SetCursor(sSavedCursor.hCursor);
                        }

                        ue.nType                = UIE_MOUSE_OUT;
                        handle_event(&ue);
                        return 0;
                    }

                    default:
                        break;
                }

                if (!bWrapper)
                    return DefWindowProcW(hWindow, uMsg, wParam, lParam);

                // If message has not been processed, update context and call previous wrappers
                SetWindowLongPtrW(hWindow, GWLP_USERDATA, pOldUserData);
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pOldProc));
                LRESULT res = (pOldProc != NULL) ?
                    pOldProc(hWindow, uMsg, wParam, lParam) :
                    DefWindowProcW(hWindow, uMsg, wParam, lParam);

                SetWindowLongPtrW(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc));

                return res;
            }

            status_t WinWindow::handle_event(const event_t *ev)
            {
                IEventHandler *handler = pHandler;

                switch (ev->nType)
                {
                    // Received window close event
                    case UIE_CLOSE:
                    {
                        if (handler == NULL)
                        {
                            this->destroy();
                            delete this;
                        }
                        break;
                    }

                    case UIE_RESIZE:
                    {
                        if (bWrapper)
                            break;

                        sSize.nLeft         = ev->nLeft;
                        sSize.nTop          = ev->nTop;
                        sSize.nWidth        = ev->nWidth;
                        sSize.nHeight       = ev->nHeight;

                        if (pSurface != NULL)
                            pSurface->sync_size();
                        break;
                    }

                    default:
                        break;
                }

                // Pass event to event handler
                if (handler != NULL)
                    handler->handle_event(ev);

                return STATUS_OK;
            }


            ISurface *WinWindow::get_surface()
            {
                return pSurface;
            }

            void *WinWindow::handle()
            {
                return reinterpret_cast<void *>(hWindow);
            }

            ssize_t WinWindow::left()
            {
                return sSize.nLeft;
            }

            ssize_t WinWindow::top()
            {
                return sSize.nTop;
            }

            ssize_t WinWindow::width()
            {
                return sSize.nWidth;
            }

            ssize_t WinWindow::height()
            {
                return sSize.nHeight;
            }

            status_t WinWindow::set_left(ssize_t left)
            {
                return move(left, sSize.nTop);
            }

            status_t WinWindow::set_top(ssize_t top)
            {
                return move(sSize.nLeft, top);
            }

            ssize_t WinWindow::set_width(ssize_t width)
            {
                return resize(width, sSize.nHeight);
            }

            ssize_t WinWindow::set_height(ssize_t height)
            {
                return resize(sSize.nWidth, height);
            }

            status_t WinWindow::hide()
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                ShowWindow(hWindow, SW_HIDE);
                return STATUS_OK;
            }

            status_t WinWindow::show()
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                ShowWindow(hWindow, SW_SHOW);
                return STATUS_OK;
            }

            status_t WinWindow::show(IWindow *over)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                HWND hTransientFor = HWND_TOP;
                if (over != NULL)
                    hTransientFor = reinterpret_cast<HWND>(over->handle());

                SetWindowPos(
                    hWindow,            // hWnd
                    hTransientFor,      // hWndInsertAfter
                    sSize.nLeft,        // X
                    sSize.nTop,         // Y
                    sSize.nWidth,       // nWidth
                    sSize.nHeight,      // nHeight
                    0                   // uFlags
                );
                ShowWindow(hWindow, SW_SHOW);

                return STATUS_OK;
            }

            status_t WinWindow::take_focus()
            {
                if ((hWindow == NULL) || (!is_visible()))
                    return STATUS_BAD_STATE;
                if (GetFocus() == hWindow)
                    return STATUS_OK;

                SetFocus(hWindow);
                return STATUS_NOT_IMPLEMENTED;
            }

            bool WinWindow::is_visible()
            {
                return (hWindow != NULL) ? IsWindowVisible(hWindow) : false;
            }

            size_t WinWindow::screen()
            {
                return (pDisplay != NULL) ? pDisplay->default_screen() : 0;
            }

            status_t WinWindow::set_caption(const LSPString *caption)
            {
                if (caption == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;
                const WCHAR *utf16 = caption->get_utf16();
                return (SetWindowTextW(hWindow, utf16)) ? STATUS_OK : STATUS_UNKNOWN_ERR;
            }

            status_t WinWindow::set_caption(const char *caption)
            {
                if (caption == NULL)
                    return STATUS_BAD_ARGUMENTS;

                LSPString tmp;
                return (tmp.set_utf8(caption)) ? set_caption(&tmp) : STATUS_NO_MEM;
            }

            status_t WinWindow::get_caption(char *text, size_t len)
            {
                if (text == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (len < 1)
                    return STATUS_TOO_BIG;

                LSPString tmp;
                status_t res = get_caption(&tmp);
                if (res != STATUS_OK)
                    return res;

                const char *utf8 = tmp.get_utf8();
                size_t count = strlen(utf8) + 1;
                if (len < count)
                    return STATUS_TOO_BIG;

                memcpy(text, utf8, count);
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_caption(LSPString *text)
            {
                if (text == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                // Obtain the length of the window text
                int length = GetWindowTextLengthW(hWindow);
                if (length == 0)
                {
                    if (GetLastError() != ERROR_SUCCESS)
                        return STATUS_UNKNOWN_ERR;
                    text->clear();
                    return STATUS_OK;
                }
                else if (length < 0)
                    return STATUS_UNKNOWN_ERR;

                // Obtain the text of the window
                WCHAR *tmp = static_cast<WCHAR *>(malloc((length + 1) * sizeof(WCHAR)));
                if (tmp == NULL)
                    return STATUS_NO_MEM;
                lsp_finally( free(tmp); );

                if (GetWindowTextW(hWindow, tmp, length + 1) <= 0)
                {
                    if (GetLastError() != ERROR_SUCCESS)
                        return STATUS_UNKNOWN_ERR;
                }

                return (text->set_utf16(tmp, length)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t WinWindow::move(ssize_t left, ssize_t top)
            {
                rectangle_t rect = sSize;
                rect.nLeft      = left;
                rect.nTop       = top;

                return set_geometry(&rect);
            }

            status_t WinWindow::resize(ssize_t width, ssize_t height)
            {
                rectangle_t rect = sSize;
                rect.nWidth     = width;
                rect.nHeight    = height;

                return set_geometry(&rect);
            }

            void WinWindow::apply_constraints(rectangle_t *dst, const rectangle_t *req)
            {
                *dst    = *req;

                if ((sConstraints.nMaxWidth >= 0) && (dst->nWidth > sConstraints.nMaxWidth))
                    dst->nWidth         = sConstraints.nMaxWidth;
                if ((sConstraints.nMaxHeight >= 0) && (dst->nHeight > sConstraints.nMaxHeight))
                    dst->nHeight        = sConstraints.nMaxHeight;
                if ((sConstraints.nMinWidth >= 0) && (dst->nWidth < sConstraints.nMinWidth))
                    dst->nWidth         = sConstraints.nMinWidth;
                if ((sConstraints.nMinHeight >= 0) && (dst->nHeight < sConstraints.nMinHeight))
                    dst->nHeight        = sConstraints.nMinHeight;
            }

            status_t WinWindow::set_geometry(const rectangle_t *realize)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                rectangle_t old = sSize;
                apply_constraints(&sSize, realize);

                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d",
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                if ((old.nLeft == sSize.nLeft) &&
                    (old.nTop == sSize.nTop) &&
                    (old.nWidth == sSize.nWidth) &&
                    (old.nHeight == sSize.nHeight))
                    return STATUS_OK;

                // These system metrics affect the actual client size of the window
                bool border             = has_border();
                ssize_t hborder         = (border) ? GetSystemMetrics(SM_CXSIZEFRAME) : 0;
                ssize_t vborder         = (border) ? GetSystemMetrics(SM_CYSIZEFRAME) : 0;
                ssize_t vcaption        = (border) ? GetSystemMetrics(SM_CYCAPTION) : 0;

                BOOL res = MoveWindow(
                    hWindow,                                // hWnd
                    sSize.nLeft,                            // X
                    sSize.nTop,                             // Y
                    sSize.nWidth + hborder * 2,             // nWidth
                    sSize.nHeight + vcaption + vborder * 2, // nHeight
                    TRUE);                                  // bRepaint

                if (res)
                    return STATUS_OK;

                lsp_error("Error moving window to l=%d, t=%d, w=%d, h=%d: error=%ld",
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight), long(GetLastError()));
                return STATUS_UNKNOWN_ERR;
            }

            status_t WinWindow::set_border_style(border_style_t style)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                return (enBorderStyle != style) ? commit_border_style(style, nActions) : STATUS_OK;
            }

            status_t WinWindow::get_border_style(border_style_t *style)
            {
                if (style == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                *style      = enBorderStyle;
                return STATUS_OK;
            }

            status_t WinWindow::set_window_actions(size_t actions)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                return (nActions != actions) ? commit_border_style(enBorderStyle, actions) : STATUS_OK;
            }

            status_t WinWindow::get_window_actions(size_t *actions)
            {
                if (actions == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                *actions    = nActions;
                return STATUS_OK;
            }

            status_t WinWindow::commit_border_style(border_style_t bs, size_t wa)
            {
                border_style_t xbs  = (has_parent()) ? BS_NONE : bs;
                size_t style        = 0;
                size_t ex_style     = 0;

                switch (xbs)
                {
                    case BS_DIALOG:
                        style       = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
                        ex_style    = WS_EX_ACCEPTFILES;
                        break;
                    case BS_SINGLE:
                        style       = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
                        if (wa & WA_MINIMIZE)
                            style       |= WS_MINIMIZEBOX;
                        if (wa & WA_MAXIMIZE)
                            style       |= WS_MAXIMIZEBOX;
                        ex_style    = WS_EX_ACCEPTFILES;
                        break;
                    case BS_SIZEABLE:
                        style       = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
                        if (wa & WA_MINIMIZE)
                            style       |= WS_MINIMIZEBOX;
                        if (wa & WA_MAXIMIZE)
                            style       |= WS_MAXIMIZEBOX;
                        ex_style    = WS_EX_ACCEPTFILES;
                        break;
                    case BS_POPUP:
                    case BS_COMBO:
                    case BS_DROPDOWN:
                        ex_style    = WS_EX_TOPMOST;
                        break;
                    case BS_NONE:
                    default:
                        style       = WS_OVERLAPPED;
                        ex_style    = WS_EX_ACCEPTFILES;
                        break;
                }

                SetWindowLongW(hWindow, GWL_STYLE, LONG(style));
                SetWindowLongW(hWindow, GWL_EXSTYLE, LONG(ex_style));

                HMENU sysmenu = (hParent == NULL) ? GetSystemMenu(hWindow, FALSE) : NULL;
                if (sysmenu != NULL)
                {
                    #define COMMIT(id, flag) \
                        EnableMenuItem(sysmenu, id, (wa & flag) ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                    COMMIT(SC_MOVE, WA_MOVE);
                    COMMIT(SC_SIZE, WA_RESIZE);
                    COMMIT(SC_MINIMIZE, WA_MINIMIZE);
                    COMMIT(SC_MAXIMIZE, WA_MAXIMIZE);
                    COMMIT(SC_CLOSE, WA_CLOSE);
                    #undef COMMIT
                }

                // Finally, update the value for fields
                enBorderStyle       = bs;
                nActions            = wa;

                return STATUS_OK;
            }

            bool WinWindow::has_border() const
            {
                if (has_parent())
                    return false;
                switch (enBorderStyle)
                {
                    case BS_DIALOG:
                    case BS_SINGLE:
                    case BS_SIZEABLE:
                        return true;
                    default:
                        break;
                }
                return false;
            }

            status_t WinWindow::get_geometry(rectangle_t *realize)
            {
                if (realize == NULL)
                    return STATUS_BAD_ARGUMENTS;

                *realize            = sSize;

                return STATUS_OK;
            }

            status_t WinWindow::get_absolute_geometry(rectangle_t *realize)
            {
                if (realize == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                POINT p;
                p.x                 = 0;
                p.y                 = 0;

                if (!ClientToScreen(hWindow, &p))
                    return STATUS_UNKNOWN_ERR;

                realize->nLeft      = p.x;
                realize->nTop       = p.y;
                realize->nWidth     = sSize.nWidth;
                realize->nHeight    = sSize.nHeight;

                return STATUS_OK;
            }

            status_t WinWindow::set_size_constraints(const size_limit_t *c)
            {
                sConstraints    = *c;
                if (sConstraints.nMinWidth == 0)
                    sConstraints.nMinWidth  = 1;
                if (sConstraints.nMinHeight == 0)
                    sConstraints.nMinHeight = 1;

                return set_geometry(&sSize);
            }

            status_t WinWindow::get_size_constraints(size_limit_t *c)
            {
                *c = sConstraints;
                return STATUS_OK;
            }

            status_t WinWindow::grab_events(grab_t group)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::ungrab_events()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_icon(const void *bgra, size_t width, size_t height)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_mouse_pointer(mouse_pointer_t pointer)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;
                if (enPointer == pointer)
                    return STATUS_OK;

                if ((bMouseInside) && (is_visible()))
                {
                    HCURSOR new_c = pDisplay->translate_cursor(pointer);
                    if (new_c == NULL)
                        return STATUS_UNKNOWN_ERR;

                    HCURSOR old_c = pDisplay->translate_cursor(enPointer);
                    if (old_c != new_c)
                    {
                        if (!SetCursor(new_c))
                            return STATUS_UNKNOWN_ERR;
                    }
                }

                // Commit the cursor value
                enPointer       = pointer;

                return STATUS_OK;
            }

            mouse_pointer_t WinWindow::get_mouse_pointer()
            {
                return enPointer;
            }

            status_t WinWindow::set_class(const char *instance, const char *wclass)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_role(const char *wrole)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            bool WinWindow::has_parent() const
            {
                if (hWindow == NULL)
                    return false;
                HWND wnd = GetParent(hWindow);

                return wnd != NULL;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */
