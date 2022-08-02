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

#ifndef PRIVATE_WIN_WINWINDOW_H_
#define PRIVATE_WIN_WINWINDOW_H_

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/win/WinDDSurface.h>
#include <private/win/dnd.h>

#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinDisplay;

            constexpr const HWND INVALID_HWND       = HWND(nullptr);

            class LSP_HIDDEN_MODIFIER WinWindow: public IWindow, public IEventHandler
            {
                private:
                    friend class WinDisplay;

                protected:
                    typedef struct btn_event_t
                    {
                        event_t         sDown;
                        event_t         sUp;
                    } btn_event_t;

                    typedef enum {
                        XKS_ALT_L       = 1 << 0,
                        XKS_ALT_R       = 1 << 1,
                        XKS_CTRL_L      = 1 << 2,
                        XKS_CTRL_R      = 1 << 3,
                        XKS_SHIFT_L     = 1 << 4,
                        XKS_SHIFT_R     = 1 << 5,
                        XKS_CAPS        = 1 << 6,
                        XKS_LBUTTON     = 1 << 7,
                        XKS_MBUTTON     = 1 << 8,
                        XKS_RBUTTON     = 1 << 9,
                        XKS_BUTTON4     = 1 << 10,
                        XKS_BUTTON5     = 1 << 11
                    } x_keystate_t;

                protected:
                    WinDisplay         *pWinDisplay;    // Pointer to the display
                    HWND                hWindow;        // The identifier of the wrapped window
                    HWND                hParent;        // The identifier of parent window
                    WinDDSurface       *pSurface;       // Drawing surface
                    WinDNDTarget       *pDNDTarget;     // Drag&Drop target
                    LONG_PTR            pOldUserData;   // Old user data
                    WNDPROC             pOldProc;       // Old window procedure (if present)
                    bool                bWrapper;       // Indicates that window is a wrapper
                    bool                bMouseInside;   // Flag that indicates that mouse is inside of the window
                    bool                bGrabbing;      // Grabbing mouse and keyboard events
                    size_t              nMouseCapture;  // Flag for capturing mouse
                    rectangle_t         sSize;          // Size of the window
                    size_limit_t        sConstraints;   // Window constraints
                    mouse_pointer_t     enPointer;      // Mouse pointer
                    border_style_t      enBorderStyle;  // Border style of the window
                    size_t              nActions;       // Allowed window actions
                    POINT               sMousePos;      // Last mouse position for tracking MOUSE_OUT event
                    CURSORINFO          sSavedCursor;   // The saved cursor before the mouse has entered the window
                    btn_event_t         vBtnEvent[3];   // Events for detecting double click and triple click

                protected:
                    void                apply_constraints(rectangle_t *dst, const rectangle_t *req);
                    void                process_mouse_event(timestamp_t ts, const ws::event_t *ev);
                    status_t            commit_border_style(border_style_t bs, size_t wa);
                    bool                has_border() const;
                    static bool         check_click(const btn_event_t *ev);
                    static bool         check_double_click(const btn_event_t *pe, const btn_event_t *ce);
                    bool                process_virtual_key(event_t *ev, WPARAM wParam, LPARAM lParam);
                    void                handle_mouse_leave();
                    void                handle_mouse_enter(const ws::event_t *ev);

                public:
                    explicit WinWindow(WinDisplay *dpy, HWND wnd, IEventHandler *handler, bool wrapper);
                    virtual ~WinWindow();

                    virtual status_t    init() override;
                    virtual void        destroy() override;

                public:
                    virtual ISurface   *get_surface() override;
                    virtual status_t    invalidate() override;
                    virtual void       *handle() override;

                    virtual ssize_t     left() override;
                    virtual ssize_t     top() override;
                    virtual ssize_t     width() override;
                    virtual ssize_t     height() override;
                    virtual status_t    set_left(ssize_t left) override;
                    virtual status_t    set_top(ssize_t top) override;
                    virtual ssize_t     set_width(ssize_t width) override;
                    virtual ssize_t     set_height(ssize_t height) override;

                    virtual status_t    hide() override;
                    virtual status_t    show() override;
                    virtual status_t    show(IWindow *over) override;
                    virtual bool        is_visible() override;

                    virtual size_t      screen() override;

                    virtual status_t    set_caption(const char *caption) override;
                    virtual status_t    set_caption(const LSPString *caption) override;
                    virtual status_t    get_caption(char *text, size_t len) override;
                    virtual status_t    get_caption(LSPString *text) override;

                    virtual status_t    move(ssize_t left, ssize_t top) override;
                    virtual status_t    resize(ssize_t width, ssize_t height) override;
                    virtual status_t    set_geometry(const rectangle_t *realize) override;
                    virtual status_t    set_border_style(border_style_t style) override;
                    virtual status_t    get_border_style(border_style_t *style) override;
                    virtual status_t    get_geometry(rectangle_t *realize) override;
                    virtual status_t    get_absolute_geometry(rectangle_t *realize) override;

                    virtual status_t    set_size_constraints(const size_limit_t *c) override;
                    virtual status_t    get_size_constraints(size_limit_t *c) override;

                    virtual status_t    get_window_actions(size_t *actions) override;
                    virtual status_t    set_window_actions(size_t actions) override;

                    virtual status_t    grab_events(grab_t group) override;
                    virtual status_t    ungrab_events() override;
                    virtual bool        is_grabbing_events() const override;

                    virtual status_t    take_focus() override;

                    virtual status_t    set_icon(const void *bgra, size_t width, size_t height) override;

                    virtual status_t    set_mouse_pointer(mouse_pointer_t pointer) override;
                    virtual mouse_pointer_t get_mouse_pointer() override;

                    virtual status_t    set_class(const char *instance, const char *wclass) override;
                    virtual status_t    set_role(const char *wrole) override;

                    virtual bool        has_parent() const override;

                public:
                    virtual status_t    handle_event(const event_t *ev) override;

                public:
                    /**
                     * Process windows event delivered to the window
                     * @param uMsg the event type
                     * @param wParam the first parameter of the event
                     * @param lParam the second parameter of the event
                     * @param ts timestamp of the event
                     * @param hook the event was delivered from the hook routine (SetWindowsHookEx)
                     * @return 0 if the event has been processed
                     */
                    LRESULT             process_event(UINT uMsg, WPARAM wParam, LPARAM lParam, timestamp_t ts, bool hook);
                    HWND                win_handle();
                    WinDisplay         *win_display();
                    WinDNDTarget       *dnd_target();
            };
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINWINDOW_H_ */
