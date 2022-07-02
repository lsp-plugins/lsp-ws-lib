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

#include <private/win/WinDisplay.h>
#include <private/win/WinWindow.h>

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
                pOldUserData            = reinterpret_cast<LONG_PTR>(reinterpret_cast<void *>(NULL));
                pOldProc                = reinterpret_cast<WNDPROC>(NULL);
                bWrapper                = wrapper;

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
                }
                else
                {
                    // Update pointer to the window routine
                    pOldProc        = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WinDisplay::window_proc)));
                    pOldUserData    = SetWindowLongPtrW(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                }

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

            LRESULT WinWindow::process_event(UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                // TODO: Translate the event and pass to handler
                event_t ue;
                init_event(&ue);

                // TODO: translate the event
                switch (uMsg)
                {
                    case WM_GETMINMAXINFO:
                    {
                        // Contains information about a window's maximized size and position and
                        // it's minimum and maximum tracking size.
                        MINMAXINFO *info    = reinterpret_cast<MINMAXINFO *>(lParam);

                        info->ptMinTrackSize.x  = lsp_max(sConstraints.nMinWidth, 1);
                        info->ptMinTrackSize.y  = lsp_max(sConstraints.nMinHeight, 1);

                        if (sConstraints.nMaxWidth >= 0)
                        {
                            ssize_t max_size        = lsp_max(sConstraints.nMaxWidth, info->ptMinTrackSize.x);
                            info->ptMaxSize.x       = max_size;
                            info->ptMaxTrackSize.x  = max_size;
                        }
                        else
                        {
                            info->ptMaxSize.x       = lsp_max(info->ptMaxSize.x, info->ptMinTrackSize.x);
                            info->ptMaxTrackSize.x  = lsp_max(info->ptMaxTrackSize.x, info->ptMinTrackSize.x);
                        }

                        if (sConstraints.nMaxHeight >= 0)
                        {
                            ssize_t max_size        = lsp_max(sConstraints.nMaxHeight, info->ptMinTrackSize.y);
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

                    case WM_DESTROY:
                    {
                        ue.nType        = UIE_CLOSE;
                        break;
                    }

                    default:
                        break;
                }

                // If we have to deliver something to the window handler, then do it
                if (ue.nType != ws::UIE_UNKNOWN)
                {
                    handle_event(&ue);
                    return 0;
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

            ISurface *WinWindow::get_surface()
            {
                return NULL;
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
                return IsWindowVisible(hWindow);
            }

            size_t WinWindow::screen()
            {
                return 0;
            }

            status_t WinWindow::set_caption(const char *ascii, const char *utf8)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_caption(char *text, size_t len)
            {
                return STATUS_NOT_IMPLEMENTED;
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

                BOOL res = MoveWindow(
                    hWindow,        // hWnd
                    sSize.nLeft,    // X
                    sSize.nTop,     // Y
                    sSize.nWidth,   // nWidth
                    sSize.nHeight,  // nHeight
                    TRUE);          // bRepaint

                if (res)
                    return STATUS_OK;

                lsp_error("Error moving window to l=%d, t=%d, w=%d, h=%d: error=%ld",
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight), long(GetLastError()));
                return STATUS_UNKNOWN_ERR;
            }

            status_t WinWindow::set_border_style(border_style_t style)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_border_style(border_style_t *style)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_geometry(rectangle_t *realize)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_absolute_geometry(rectangle_t *realize)
            {
                return STATUS_NOT_IMPLEMENTED;
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

            status_t WinWindow::get_window_actions(size_t *actions)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_window_actions(size_t actions)
            {
                return STATUS_NOT_IMPLEMENTED;
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

                    default:
                        break;
                }

                // Pass event to event handler
                if (handler != NULL)
                    handler->handle_event(ev);

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

            status_t WinWindow::set_mouse_pointer(mouse_pointer_t ponter)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            mouse_pointer_t WinWindow::get_mouse_pointer()
            {
                return MP_NONE;
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
                return false;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */
