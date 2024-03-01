/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/ws/keycodes.h>

#include <private/win/WinDisplay.h>
#include <private/win/WinDDSurface.h>
#include <private/win/WinWindow.h>

#include <windows.h>
#include <windowsx.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            WinWindow::WinWindow(WinDisplay *dpy, HWND wnd, IEventHandler *handler, bool wrapper):
                ws::IWindow(dpy, handler)
            {
                pWinDisplay             = dpy;
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
                hTransientFor           = NULL;
                pSurface                = NULL;
                pDNDTarget              = NULL;
                pOldUserData            = reinterpret_cast<LONG_PTR>(reinterpret_cast<void *>(NULL));
                pOldProc                = static_cast<WNDPROC>(NULL);
                bWrapper                = wrapper;
                bMouseInside            = false;
                bGrabbing               = false;
                bTransientOn            = false;
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

                sMousePos.x             = 0;
                sMousePos.y             = 0;
                bzero(&sSavedCursor, sizeof(sSavedCursor));

                for (size_t i=0; i<3; ++i)
                {
                    init_event(&vBtnEvent[i].sDown);
                    init_event(&vBtnEvent[i].sUp);
                    vBtnEvent[i].sDown.nType    = UIE_UNKNOWN;
                    vBtnEvent[i].sUp.nType      = UIE_UNKNOWN;
                }
            }

            WinWindow::~WinWindow()
            {
                pDisplay        = NULL;
            }

            status_t WinWindow::init()
            {
                // Register the window
                if (pWinDisplay == NULL)
                    return STATUS_BAD_STATE;

                // Initialize parent class
                status_t res = IWindow::init();
                if (res != STATUS_OK)
                    return res;

                if (!bWrapper)
                {
                    // Create new window
                    hWindow         = CreateWindowExW(
                        0,                                                  // dwExStyle
                        pWinDisplay->sWindowClassName.get_utf16(),          // lpClassName
                        L"",                                                // lpWindowName
                        (hParent != NULL) ? WS_CHILDWINDOW : WS_OVERLAPPEDWINDOW, // dwStyle
                        sSize.nLeft,                                        // X
                        sSize.nTop,                                         // Y
                        sSize.nWidth,                                       // nWidth
                        sSize.nHeight,                                      // nHeight
                        hParent,                                            // hWndParent
                        NULL,                                               // hMenu
                        GetModuleHandleW(NULL),                             // hInstance
                        this                                                // lpCreateParam
                    );

                    if (hWindow == NULL)
                    {
                        lsp_error("Error creating window: %ld", long(GetLastError()));
                        return STATUS_UNKNOWN_ERR;
                    }
                    lsp_trace("Created window hWND=%p", hWindow);

                    commit_border_style(enBorderStyle, nActions);
                }
                else
                {
                    // Update pointer to the window routine
                    pOldProc        = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc)));
                    pOldUserData    = SetWindowLongPtrW(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

                    lsp_trace("pOldProc = %p, pOldUserData = 0x%lx",  pOldProc, pOldUserData);
                }

                // Create Surface
                pSurface        = new WinDDSurface(pWinDisplay, hWindow, sSize.nWidth, sSize.nHeight);
                if (pSurface == NULL)
                    return STATUS_NO_MEM;

                // Create Drag&Drop target
                pDNDTarget      = new WinDNDTarget(this);
                if (pDNDTarget == NULL)
                    return STATUS_NO_MEM;
                pDNDTarget->AddRef();
                HRESULT hr = RegisterDragDrop(hWindow, pDNDTarget);
                if (FAILED(hr))
                {
                    lsp_error("Failed to register Drag&Drop target: result=0x%lx", long(hr));
                    return STATUS_UNKNOWN_ERR;
                }

                // Enable all keyboard and mouse input for the window
                EnableWindow(hWindow, TRUE);

                return STATUS_OK;
            }

            void WinWindow::destroy()
            {
                if (hWindow == NULL)
                    return;

                // Drag&Drop related
                if (pWinDisplay != NULL)
                    pWinDisplay->unset_drag_window(this);
                if (pDNDTarget != NULL)
                {
                    RevokeDragDrop(hWindow);

                    pDNDTarget->Release();
                    pDNDTarget  = NULL;
                }

                // Surface related
                if (pSurface != NULL)
                {
                    pSurface->destroy();
                    delete pSurface;
                    pSurface = NULL;
                }

                // Custom user data
                SetWindowLongPtrW(hWindow, GWLP_USERDATA, pOldUserData);
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc));

                if (!bWrapper)
                    DestroyWindow(hWindow);

                hWindow     = NULL;
                pWinDisplay = NULL;

                IWindow::destroy();
            }

            void WinWindow::handle_mouse_leave()
            {
                ws::event_t ue;

                bMouseInside            = false;
                ue.nType                = UIE_MOUSE_OUT;
                ue.nLeft                = sMousePos.x;
                ue.nTop                 = sMousePos.y;

                // Restore the previous cursor if it was saved
                if (sSavedCursor.cbSize == sizeof(sSavedCursor))
                {
                    HCURSOR cursor = pWinDisplay->translate_cursor(enPointer);
                    if (cursor != NULL)
                        SetCursor(sSavedCursor.hCursor);
                }

                handle_event(&ue);
            }

            void WinWindow::handle_mouse_enter(const ws::event_t *ev)
            {
                ws::event_t xev     = *ev;
                xev.nType           = UIE_MOUSE_IN;
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
                    HCURSOR cursor = pWinDisplay->translate_cursor(enPointer);
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
                    sSavedCursor.cbSize     = 0;
                    xev.nLeft               = 0;
                    xev.nTop                = 0;
                }

                // Notify the handler about mouse enter event
                handle_event(&xev);
            }

            void WinWindow::process_mouse_event(timestamp_t ts, const ws::event_t *ev)
            {
                // Save current mouse position
                sMousePos.x         = LONG(ev->nLeft);
                sMousePos.y         = LONG(ev->nTop);

                RECT rect;
                if (!GetClientRect(hWindow, &rect))
                {
                    rect.left       = 0;
                    rect.top        = 0;
                    rect.right      = sSize.nWidth - 1;
                    rect.bottom     = sSize.nHeight - 1;
                }

                if ((sMousePos.x < rect.left) ||
                    (sMousePos.y < rect.top) ||
                    (sMousePos.x > rect.right) ||
                    (sMousePos.y > rect.bottom))
                {
                    // Generate mouse out event if necessary
                    if (bMouseInside)
                        handle_mouse_leave();
                }
                else
                {
                    if (!bMouseInside)
                        handle_mouse_enter(ev);
                }
            }

            LRESULT WinWindow::process_event(UINT uMsg, WPARAM wParam, LPARAM lParam, timestamp_t ts, bool hook)
            {
                // Translate the event and pass to handler
                event_t ue;
                init_event(&ue);
                ue.nTime        = ts;

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

                        // Render and re-render if the surface is invalid
                        handle_event(&ue);
                        if (!pSurface->valid())
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

                        process_mouse_event(ts, &ue);
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
                        ue.nState               = decode_mouse_keystate(wParam) ^ (1 << ue.nCode);

                        // Set mouse capture
                        if (!nMouseCapture)
                            SetCapture(hWindow);
                        nMouseCapture          |= 1 << ue.nCode;

                        process_mouse_event(ts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_XBUTTONDOWN:
                    {
                        ue.nType                = UIE_MOUSE_DOWN;
                        ue.nCode                = (HIWORD(wParam) == XBUTTON1) ? MCB_BUTTON4 :
                                                  (HIWORD(wParam) == XBUTTON2) ? MCB_BUTTON5 :
                                                  MCB_NONE;
                        if (ue.nCode == MCB_NONE)
                            break;

                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);
                        ue.nState               = decode_mouse_keystate(wParam) ^ (1 << ue.nCode);

                        // Set mouse capture
                        if (!nMouseCapture)
                            SetCapture(hWindow);
                        nMouseCapture          |= 1 << ue.nCode;

                        process_mouse_event(ts, &ue);
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
                        ue.nState               = decode_mouse_keystate(wParam) ^ (1 << ue.nCode);

                        // Reset capture if possible
                        nMouseCapture          &= ~(1 << ue.nCode);
                        if (nMouseCapture == 0)
                            ReleaseCapture();

                        process_mouse_event(ts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_XBUTTONUP:
                    {
                        ue.nType                = UIE_MOUSE_UP;
                        ue.nCode                = (HIWORD(wParam) == XBUTTON1) ? MCB_BUTTON4 :
                                                  (HIWORD(wParam) == XBUTTON2) ? MCB_BUTTON5 :
                                                  MCB_NONE;
                        if (ue.nCode == MCB_NONE)
                            break;

                        ue.nLeft                = GET_X_LPARAM(lParam);
                        ue.nTop                 = GET_Y_LPARAM(lParam);
                        ue.nState               = decode_mouse_keystate(wParam) ^ (1 << ue.nCode);

                        // Reset capture if possible
                        nMouseCapture          &= ~(1 << ue.nCode);
                        if (nMouseCapture == 0)
                            ReleaseCapture();

                        process_mouse_event(ts, &ue);
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

                        // Convert screen coordinates into window coordinates
                        POINT pt;
                        pt.x                    = GET_X_LPARAM(lParam);
                        pt.y                    = GET_Y_LPARAM(lParam);
                        ScreenToClient(hWindow, &pt);

                        // Generate the mouse event and process it
                        ue.nState               = decode_mouse_keystate(GET_KEYSTATE_WPARAM(wParam));
                        ue.nLeft                = pt.x;
                        ue.nTop                 = pt.y;

                        process_mouse_event(ts, &ue);
                        handle_event(&ue);
                        return 0;
                    }
                    case WM_MOUSELEAVE:
                    {
                        lsp_trace("WM_MOUSELEAVE");
                        if (hook == bGrabbing)
                            handle_mouse_leave();
                        return 0;
                    }

                    // Keyboard events
                    case WM_KEYUP:
                    case WM_SYSKEYUP:
                    {
                        ue.nType                = UIE_KEY_UP;
                        if (process_virtual_key(&ue, wParam, lParam))
                            handle_event(&ue);
                        return 0;
                    }

                    case WM_KEYDOWN:
                    case WM_SYSKEYDOWN:
                    {
                        if (!process_virtual_key(&ue, wParam, lParam))
                            return 0;

                        // If the key was pressed, we need to simulate UIE_KEY_UP event
                        if (lParam & (1 << 30))
                        {
                            ue.nType                = UIE_KEY_UP;
                            handle_event(&ue);
                        }

                        // Now send KEY_DOWN event
                        ue.nType                = ((uMsg == WM_KEYDOWN) || (uMsg == WM_SYSKEYDOWN)) ? UIE_KEY_DOWN : UIE_KEY_UP;
                        handle_event(&ue);
                        return 0;
                    }

                    case WM_WINDOWPOSCHANGING:
                    {
                        if ((hTransientFor == NULL) || (hTransientFor == HWND_TOP))
                            break;

//                        WCHAR caption[256];
//                        LSPString tmp;
//
//                        lsp_trace("Order for hWnd = %p, hTransientFor = %p:", hWindow, hTransientFor);
//                        HWND hIter  = GetWindow(hWindow, GW_HWNDFIRST);
//                        while (hIter != NULL)
//                        {
//                            caption[0] = 0;
//                            GetWindowTextW(hIter, caption, sizeof(caption)/sizeof(WCHAR));
//                            tmp.set_utf16(caption);
//                            lsp_trace("  hWnd=%p (%s)", hIter, tmp.get_native());
//
//                            hIter   = GetWindow(hIter, GW_HWNDNEXT);
//                        }

                        HWND hPrev          = GetWindow(hWindow, GW_HWNDFIRST);
//                        lsp_trace("hPrev = %p", hPrev);
                        while (hPrev != NULL)
                        {
                            HWND hCurr              = GetWindow(hPrev, GW_HWNDNEXT);
//                            lsp_trace("hCurr = %p", hCurr);

                            // We are already at the top?
                            if (hCurr == hWindow)
                                break;
                            if (hCurr == hTransientFor)
                            {
//                                lsp_trace("overriding hwndInsertAfter with %p", hPrev);
                                WINDOWPOS *p            = reinterpret_cast<WINDOWPOS *>(lParam);
                                p->hwndInsertAfter      = hPrev;
                                p->flags               &= ~SWP_NOZORDER;
                                break;
                            }

                            // Update pointer
                            hPrev                  = hCurr;
                        }

                        break;
                    }

//                    case WM_CHAR:
//                        lsp_trace("WM_CHAR code=0x%x", wParam);
//                        return 0;
//                    case WM_DEADCHAR:
//                        lsp_trace("WM_DEADCHAR code=0x%x", wParam);
//                        return 0;

                    default:
                        break;
                }

                // Do not call default window procedure if it is a hook-based call
                if (hook)
                    return 0;

                if (!bWrapper)
                    return DefWindowProcW(hWindow, uMsg, wParam, lParam);

                // If message has not been processed, update context and call previous wrappers
                lsp_trace("pOldProc = %p, pOldUserData=0x%lx", pOldProc, pOldUserData);

                SetWindowLongPtrW(hWindow, GWLP_USERDATA, pOldUserData);
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pOldProc));

                LRESULT res = (pOldProc != NULL) ?
                    pOldProc(hWindow, uMsg, wParam, lParam) :
                    DefWindowProcW(hWindow, uMsg, wParam, lParam);

                SetWindowLongPtrW(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc));

                return res;
            }

            bool WinWindow::check_click(const btn_event_t *ev)
            {
                if ((ev->sDown.nType != UIE_MOUSE_DOWN) || (ev->sUp.nType != UIE_MOUSE_UP))
                    return false;
                if (ev->sDown.nCode != ev->sUp.nCode)
                    return false;

                UINT delay = GetDoubleClickTime();
                if ((ev->sUp.nTime < ev->sDown.nTime) || ((ev->sUp.nTime - ev->sDown.nTime) > delay))
                    return false;

                return (ev->sDown.nLeft == ev->sUp.nLeft) && (ev->sDown.nTop == ev->sUp.nTop);
            }

            bool WinWindow::check_double_click(const btn_event_t *pe, const btn_event_t *ev)
            {
                if (!check_click(pe))
                    return false;

                if (pe->sDown.nCode != ev->sDown.nCode)
                    return false;
                UINT delay = GetDoubleClickTime();
                if ((ev->sUp.nTime < pe->sUp.nTime) || ((ev->sUp.nTime - pe->sUp.nTime) > delay))
                    return false;

                return (pe->sUp.nLeft == ev->sUp.nLeft) && (pe->sUp.nTop == ev->sUp.nTop);
            }

            status_t WinWindow::handle_event(const event_t *ev)
            {
                IEventHandler *handler = pHandler;

                // Generated event
                event_t gen;
                init_event(&gen);

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

                    // Simulation of mouse double click and triple click events
                    case UIE_MOUSE_DOWN:
                    {
                        // Shift the buffer and push event
                        vBtnEvent[0]            = vBtnEvent[1];
                        vBtnEvent[1]            = vBtnEvent[2];
                        vBtnEvent[2].sDown      = *ev;
                        init_event(&vBtnEvent[2].sUp);
                        break;
                    }

                    case UIE_MOUSE_UP:
                    {
                        // Push event
                        vBtnEvent[2].sUp        = *ev;

                        // Check that click happened
                        if (check_click(&vBtnEvent[2]))
                        {
                            gen                     = *ev;
                            gen.nType               = UIE_MOUSE_CLICK;

                            if (check_double_click(&vBtnEvent[1], &vBtnEvent[2]))
                            {
                                gen.nType               = UIE_MOUSE_DBL_CLICK;
                                if (check_double_click(&vBtnEvent[0], &vBtnEvent[1]))
                                    gen.nType               = UIE_MOUSE_TRI_CLICK;
                            }
                        }
                        break;
                    }

                    // Drag&Drop
                    case UIE_DRAG_ENTER:
                        pWinDisplay->set_drag_window(this);
                        break;
                    case UIE_DRAG_LEAVE:
                        pWinDisplay->unset_drag_window(this);
                        break;
                    case UIE_DRAG_REQUEST:
                        // Skip drag events which are not related to this window
                        if (pWinDisplay->drag_window() != this)
                            return STATUS_OK;
                        break;

                    default:
                        break;
                }

                // Pass event to event handler
                if (handler != NULL)
                {
                    handler->handle_event(ev);
                    if (gen.nType != UIE_UNKNOWN)
                        handler->handle_event(&gen);
                }

                return STATUS_OK;
            }


            ISurface *WinWindow::get_surface()
            {
                return pSurface;
            }

            status_t WinWindow::invalidate()
            {
                if ((hWindow == NULL) || (!is_visible()))
                    return STATUS_BAD_STATE;

                InvalidateRect(hWindow, NULL, FALSE);
                return STATUS_OK;
            }

            void *WinWindow::handle()
            {
                return reinterpret_cast<void *>(hWindow);
            }

            HWND WinWindow::win_handle()
            {
                return hWindow;
            }

            WinDisplay *WinWindow::win_display()
            {
                return pWinDisplay;
            }

            WinDNDTarget *WinWindow::dnd_target()
            {
                return pDNDTarget;
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
                lsp_trace("Hide window this=%p", this);

                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                if (bGrabbing)
                {
                    pWinDisplay->ungrab_events(this);
                    bGrabbing = false;
                }

                if ((hTransientFor != HWND_TOP) && (hTransientFor != NULL))
                {
                    EnableWindow(hTransientFor, bTransientOn);
                    hTransientFor   = NULL;
                }
                ShowWindow(hWindow, SW_HIDE);
                return STATUS_OK;
            }

            status_t WinWindow::show()
            {
                lsp_trace("Show window this=%p, hWindow=%p", this, hWindow);
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                hTransientFor       = NULL;
                ShowWindow(hWindow, SW_SHOW);
                return STATUS_OK;
            }

            status_t WinWindow::show(IWindow *over)
            {
                lsp_trace("Show window this=%p, hWindow=%p over=%p", this, hWindow, over);

                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                hTransientFor   = HWND_TOP;
                if (over != NULL)
                {
                    hTransientFor   = reinterpret_cast<HWND>(over->handle());
                    bTransientOn    = !EnableWindow(hTransientFor, FALSE);
                }

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
                lsp_finally{ free(tmp); };

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

//                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d",
//                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

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
                size_t style    = 0;
                size_t ex_style = 0;
                HMENU sysmenu   = NULL;

                if (has_parent())
                {
                    style           = WS_CHILDWINDOW;
                    ex_style        = WS_EX_ACCEPTFILES;
                    sysmenu         = NULL;
                }
                else
                {
                    sysmenu             = GetSystemMenu(hWindow, FALSE);

                    switch (bs)
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
                            style       = WS_POPUP;
                            ex_style    = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
                            break;
                        case BS_NONE:
                        default:
                            style       = WS_OVERLAPPED;
                            ex_style    = WS_EX_ACCEPTFILES;
                            break;
                    }
                }

                SetWindowLongW(hWindow, GWL_STYLE, LONG(style));
                SetWindowLongW(hWindow, GWL_EXSTYLE, LONG(ex_style));

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
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                if (bGrabbing)
                    return STATUS_OK;

                status_t res = pWinDisplay->grab_events(this, group);
                if (res == STATUS_OK)
                    bGrabbing = true;

                return res;
            }

            status_t WinWindow::ungrab_events()
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;
                if (!bGrabbing)
                    return STATUS_NO_GRAB;

                status_t res = pWinDisplay->ungrab_events(this);
                bGrabbing = false;

                // Set mouse tracking
                TRACKMOUSEEVENT track;
                track.cbSize        = sizeof(track);
                track.dwFlags       = TME_LEAVE;
                track.hwndTrack     = hWindow;
                track.dwHoverTime   = 0;
                TrackMouseEvent(&track);

                return res;
            }

            bool WinWindow::is_grabbing_events() const
            {
                return bGrabbing;
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
                    HCURSOR new_c = pWinDisplay->translate_cursor(pointer);
                    if (new_c == NULL)
                        return STATUS_UNKNOWN_ERR;

                    HCURSOR old_c = pWinDisplay->translate_cursor(enPointer);
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

            void *WinWindow::parent() const
            {
                if (hWindow == NULL)
                    return NULL;

                return reinterpret_cast<void *>(hParent);
            }

            status_t WinWindow::set_parent(void *parent)
            {
                if (hWindow == NULL)
                    return STATUS_BAD_STATE;

                HWND pwindow = reinterpret_cast<HWND>(parent);
                if (SetParent(hWindow, pwindow) == NULL)
                    return STATUS_UNKNOWN_ERR;

                hParent = pwindow;
                commit_border_style(enBorderStyle, nActions);

                return STATUS_OK;
            }

            bool WinWindow::process_virtual_key(event_t *ev, WPARAM wParam, LPARAM lParam)
            {
                #define TRK(scan, code) \
                    case scan: \
                        ev->nCode = code; \
                        return true;

                #define TRX(scan, cond, code1, code2) \
                    case scan: \
                        ev->nCode = (cond) ? code2 : code1; \
                        return true;

                #define SKP(scan) \
                    case scan: \
                        break;

                #define IGN(scan) \
                    case scan: \
                        return false;

                BYTE kState[256];
                if (!GetKeyboardState(kState))
                    return 0;

                // lParam bit #29
                // The context code. The value is 1 if the ALT key is down while the key is pressed;
                // it is 0 if the WM_SYSKEYDOWN message is posted to the active window because no window
                // has the keyboard focus.
                ev->nState          = decode_kb_keystate(kState);
                if (lParam & (1 << 29))
                    ev->nState     |= MCF_ALT;

                // lParam bit #24
                // Indicates whether the key is an extended key, such as the right-hand ALT and CTRL keys
                // that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended
                // key; otherwise, it is 0.
                bool ext            = ev->nState & (1 << 24);

                ev->nCode           = WSK_UNKNOWN;
                ev->nRawCode        = (lParam & (~0xffff)) | wParam;

                //lsp_trace("wparam=0x%x, lparam=0x%x, ext=%d", int(wParam), int(lParam), int(ext));

                // Process special case for numpad
                bool num = kState[VK_NUMLOCK];
                switch (wParam)
                {
                    TRK(VK_CANCEL, WSK_BREAK)  // Control-break processing

                    TRK(VK_BACK, WSK_BACKSPACE)
                    TRK(VK_TAB, WSK_TAB)

                    TRK(VK_CLEAR, WSK_CLEAR)
                    TRK(VK_RETURN, WSK_RETURN)

                    TRX(VK_SHIFT, ext, WSK_SHIFT_L, WSK_SHIFT_R)
                    TRX(VK_CONTROL, ext, WSK_CONTROL_L, WSK_CONTROL_R)
                    TRX(VK_MENU, ext, WSK_ALT_L, WSK_ALT_R)

                    TRK(VK_PAUSE, WSK_PAUSE);
                    TRK(VK_CAPITAL, WSK_CAPS_LOCK);

                    TRK(VK_ESCAPE, WSK_ESCAPE)

                    TRK(VK_PRIOR, WSK_PAGE_UP)
                    TRK(VK_NEXT, WSK_PAGE_DOWN)
                    TRK(VK_END, WSK_END)
                    TRK(VK_HOME, WSK_HOME)
                    TRK(VK_LEFT, WSK_LEFT)
                    TRK(VK_UP, WSK_UP)
                    TRK(VK_RIGHT, WSK_RIGHT)
                    TRK(VK_DOWN, WSK_DOWN)
                    TRK(VK_SELECT, WSK_SELECT)
                    TRK(VK_PRINT, WSK_PRINT)
                    TRK(VK_EXECUTE, WSK_EXECUTE)
                    TRK(VK_SNAPSHOT, WSK_SYS_REQ)
                    TRK(VK_INSERT, WSK_INSERT)
                    TRK(VK_DELETE, WSK_DELETE)
                    TRK(VK_HELP, WSK_HELP)

                    TRK(VK_LWIN, WSK_SUPER_L)
                    TRK(VK_RWIN, WSK_SUPER_R)
                    TRK(VK_APPS, WSK_MENU)
//                    TRK(VK_SLEEP, WSK_HYPER_L) // TODO: check
                    TRK(VK_SLEEP, WSK_HYPER_R)  // TODO: check

                    TRK(VK_F1, WSK_F1)
                    TRK(VK_F2, WSK_F2)
                    TRK(VK_F3, WSK_F3)
                    TRK(VK_F4, WSK_F4)
                    TRK(VK_F5, WSK_F5)
                    TRK(VK_F6, WSK_F6)
                    TRK(VK_F7, WSK_F7)
                    TRK(VK_F8, WSK_F8)
                    TRK(VK_F9, WSK_F9)
                    TRK(VK_F10, WSK_F10)
                    TRK(VK_F11, WSK_F11)
                    TRK(VK_F12, WSK_F12)
                    TRK(VK_F13, WSK_F13)
                    TRK(VK_F14, WSK_F14)
                    TRK(VK_F15, WSK_F15)
                    TRK(VK_F16, WSK_F16)
                    TRK(VK_F17, WSK_F17)
                    TRK(VK_F18, WSK_F18)
                    TRK(VK_F19, WSK_F19)
                    TRK(VK_F20, WSK_F20)
                    TRK(VK_F21, WSK_F21)
                    TRK(VK_F22, WSK_F22)
                    TRK(VK_F23, WSK_F23)
                    TRK(VK_F24, WSK_F24)

                    TRK(VK_NUMLOCK, WSK_NUM_LOCK)
                    TRK(VK_SCROLL, WSK_SCROLL_LOCK)

                    TRK(VK_LSHIFT, WSK_SHIFT_L)
                    TRK(VK_RSHIFT, WSK_SHIFT_R)
                    TRK(VK_LCONTROL, WSK_CONTROL_L)
                    TRK(VK_RCONTROL, WSK_CONTROL_R)
                    TRK(VK_LMENU, WSK_ALT_L)
                    TRK(VK_RMENU, WSK_ALT_R)

                    TRX(VK_NUMPAD0, num, WSK_KEYPAD_0, WSK_KEYPAD_INSERT)
                    TRX(VK_NUMPAD1, num, WSK_KEYPAD_1, WSK_KEYPAD_END)
                    TRX(VK_NUMPAD2, num, WSK_KEYPAD_2, WSK_KEYPAD_DOWN)
                    TRX(VK_NUMPAD3, num, WSK_KEYPAD_3, WSK_KEYPAD_PAGE_UP)
                    TRX(VK_NUMPAD4, num, WSK_KEYPAD_4, WSK_KEYPAD_LEFT)
                    TRX(VK_NUMPAD5, num, WSK_KEYPAD_5, WSK_KEYPAD_BEGIN)
                    TRX(VK_NUMPAD6, num, WSK_KEYPAD_6, WSK_KEYPAD_RIGHT)
                    TRX(VK_NUMPAD7, num, WSK_KEYPAD_7, WSK_KEYPAD_HOME)
                    TRX(VK_NUMPAD8, num, WSK_KEYPAD_8, WSK_KEYPAD_UP)
                    TRX(VK_NUMPAD9, num, WSK_KEYPAD_9, WSK_KEYPAD_PAGE_UP)
                    TRK(VK_MULTIPLY, WSK_KEYPAD_MULTIPLY)
                    TRK(VK_ADD, WSK_KEYPAD_ADD)
                    TRK(VK_SEPARATOR, WSK_KEYPAD_SEPARATOR)
                    TRK(VK_SUBTRACT, WSK_KEYPAD_SUBTRACT)
                    TRX(VK_DECIMAL, num, WSK_KEYPAD_DECIMAL, WSK_KEYPAD_DELETE)
                    TRK(VK_DIVIDE, WSK_KEYPAD_DIVIDE)
                    default:
                        break;
                }

                WCHAR buf[4];
                UINT wScanCode = MapVirtualKeyW(wParam, MAPVK_VK_TO_VSC);

                kState[VK_CONTROL]  = 0;
                kState[VK_LCONTROL]  = 0;
                kState[VK_RCONTROL]  = 0;
                int res = ToUnicode(wParam, wScanCode, kState, buf, sizeof(buf)/sizeof(WCHAR), MAPVK_VK_TO_VSC);
                if (res == 1)
                {
                    ev->nCode           = buf[0];
                    return true;
                }
                else if (res >= 2)
                {
                    // Surrogate pair?
                    WCHAR p1 = buf[0] & 0xfc00;
                    WCHAR p2 = buf[1] & 0xfc00;

                    if (p1 == 0xd800) // buf[0] is high part of surrogate pair
                    {
                        if (p2 == 0xdc00) // buf[1] is low part of surrogate pair
                            ev->nCode           = 0x10000 + (((buf[0] & 0x3ff) << 10) | (buf[1] & 0x3ff));
                        else
                            ev->nCode           = WSK_UNKNOWN;
                    }
                    else if (p1 == 0xdc00) // buf[0] is low part of surrogate pair
                    {
                        if (p2 == 0xd800) // buf[1] is high part of surrogate pair
                            ev->nCode           = 0x10000 + (((buf[1] & 0x3ff) << 10) | (buf[0] & 0x3ff));
                        else
                            ev->nCode           = WSK_UNKNOWN;
                    }
                    else
                        ev->nCode           = WSK_UNKNOWN;

                    return true;
                }

                // Could not translate the key, analyze the code
                switch (wParam)
                {
                    IGN(VK_LBUTTON)         // Left mouse button
                    IGN(VK_RBUTTON)         // Right mouse button

                    IGN(VK_MBUTTON)         // Middle mouse button
                    IGN(VK_XBUTTON1)        // X1 mouse button
                    IGN(VK_XBUTTON2)        // X2 mouse button

                    // TODO: lookup how to deal with it
                    IGN(VK_KANA| VK_HANGEUL | VK_HANGUL)    // IME Kana mode, IME Hanguel mode (maintained for compatibility; use VK_HANGUL), IME Hangul mode
                    IGN(VK_IME_ON)          // IME On
                    IGN(VK_JUNJA)           // IME Junja mode
                    IGN(VK_FINAL)           // IME final mode
                    IGN(VK_HANJA | VK_KANJI)// IME Hanja mode, IME Kanji mode
                    IGN(VK_IME_OFF)         // IME Off

                    // TODO: lookup how to deal with it
                    IGN(VK_CONVERT)         // IME convert
                    IGN(VK_NONCONVERT)      // IME nonconvert
                    IGN(VK_ACCEPT)          // IME accept
                    IGN(VK_MODECHANGE)      // IME mode change request

                    // TODO: find out how to deal with it
                    IGN(VK_BROWSER_BACK)        // Browser Back key
                    IGN(VK_BROWSER_FORWARD)     // Browser Forward key
                    IGN(VK_BROWSER_REFRESH)     // Browser Refresh key
                    IGN(VK_BROWSER_STOP)        // Browser Stop key
                    IGN(VK_BROWSER_SEARCH)      // Browser Search key
                    IGN(VK_BROWSER_FAVORITES)   // Browser Favorites key
                    IGN(VK_BROWSER_HOME)        // Browser Start and Home key
                    IGN(VK_VOLUME_MUTE)         // Volume Mute key
                    IGN(VK_VOLUME_DOWN)         // Volume Down key
                    IGN(VK_VOLUME_UP)           // Volume Up key
                    IGN(VK_MEDIA_NEXT_TRACK)    // Next Track key
                    IGN(VK_MEDIA_PREV_TRACK)    // Previous Track key
                    IGN(VK_MEDIA_STOP)          // Stop Media key
                    IGN(VK_MEDIA_PLAY_PAUSE)    // Play/Pause Media key
                    IGN(VK_LAUNCH_MAIL)         // Start Mail key
                    IGN(VK_LAUNCH_MEDIA_SELECT) // Select Media key
                    IGN(VK_LAUNCH_APP1)         // Start Application 1 key
                    IGN(VK_LAUNCH_APP2)         // Start Application 2 key

                    SKP(VK_OEM_1)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
                    SKP(VK_OEM_PLUS)            // For any country/region, the '+' key
                    SKP(VK_OEM_COMMA)           // For any country/region, the ',' key
                    SKP(VK_OEM_MINUS)           // For any country/region, the '-' key
                    SKP(VK_OEM_PERIOD)          // For any country/region, the '.' key
                    SKP(VK_OEM_2)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
                    SKP(VK_OEM_3)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
                    SKP(VK_OEM_4)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
                    SKP(VK_OEM_5)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
                    SKP(VK_OEM_6)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
                    SKP(VK_OEM_7)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
                    SKP(VK_OEM_8)               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
                    SKP(VK_OEM_102)             // The <> keys on the US standard keyboard, or the \\| key on the non-US 102-key keyboard

                    // TODO: how to deal with it?
                    IGN(VK_PROCESSKEY)          // IME PROCESS key
                    IGN(VK_PACKET)              // Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP

                    IGN(VK_ATTN)                // Attn key
                    IGN(VK_CRSEL)               // CrSel key
                    IGN(VK_EXSEL)               // ExSel key
                    IGN(VK_EREOF)               // Erase EOF key
                    IGN(VK_PLAY)                // Play key
                    IGN(VK_ZOOM)                // Zoom key
                    IGN(VK_NONAME)              // Reserved
                    IGN(VK_PA1)                 // PA1 key
                    IGN(VK_OEM_CLEAR)           // Clear key

                    default:
                        break;
                } // switch

                #undef TRK
                #undef TRX
                #undef SKP
                #undef IGN

                return false;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */
