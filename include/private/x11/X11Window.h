/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 10 окт. 2016 г.
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

#ifndef UI_X11_WINDOW_H_
#define UI_X11_WINDOW_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ws/IEventHandler.h>
#include <lsp-plug.in/ws/IWindow.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class X11Display;

            class X11Window: public IWindow, public IEventHandler
            {
                protected:
                    enum flags_t
                    {
                        F_GRABBING      = 1 << 0,
                        F_LOCKING       = 1 << 1
                    };

                    typedef struct btn_event_t
                    {
                        event_t         sDown;
                        event_t         sUp;
                    } btn_event_t;

                public:
                    X11Display         *pX11Display;
                    ::Window            hWindow;
                    ::Window            hParent;
                    ::Window            hTransientFor;
                    ISurface           *pSurface;
                    border_style_t      enBorderStyle;
                    motif_hints_t       sMotif;
                    size_t              nActions;
                    size_t              nScreen;
                    size_t              nFlags;
                    mouse_pointer_t     enPointer;
                    bool                bWrapper;
                    bool                bVisible;

                    rectangle_t         sSize;
                    size_limit_t        sConstraints;
                    btn_event_t         vBtnEvent[3];

                protected:
                    void                drop_surface();
                    void                do_create();
                    static bool         check_click(const btn_event_t *ev);
                    static bool         check_double_click(const btn_event_t *pe, const btn_event_t *ce);
                    void                send_focus_event();
                    status_t            commit_size();

                protected:

                    void                calc_constraints(rectangle_t *dst, const rectangle_t *req);

                    status_t            do_update_constraints(bool disable=false);

                public:
                    explicit X11Window(X11Display *core, size_t screen, ::Window wnd, IEventHandler *handler, bool wrapper);
                    virtual ~X11Window();

                    /** Window initialization routine
                     *
                     * @return status of operation
                     */
                    virtual status_t    init();

                    /** Window finalization routine
                     *
                     */
                    virtual void        destroy();

                public:
                    /** Get event handler
                     *
                     * @return event handler
                     */
                    inline IEventHandler *get_handler() { return pHandler; }

                    /** Get surface for drawing
                     *
                     * @return surface for drawing
                     */
                    virtual ISurface *get_surface();

                    /** Get left coordinate of window
                     *
                     * @return value
                     */
                    virtual ssize_t left();

                    /** Get top coordinate of window
                     *
                     * @return value
                     */
                    virtual ssize_t top();

                    /** Get width of the window
                     *
                     * @return value
                     */
                    virtual ssize_t width();

                    /** Get height of the window
                     *
                     * @return value
                     */
                    virtual ssize_t height();

                    /** Get window visibility
                     *
                     * @return true if window is visible
                     */
                    virtual bool is_visible();

                    /** Get native handle
                     *
                     */
                    virtual void *handle();

                    /** Get window's screen
                     *
                     * @return window's screen
                     */
                    virtual size_t screen();

                    virtual status_t set_caption(const char *ascii, const char *utf8);

                    inline ::Window x11handle() const { return hWindow; }

                    inline ::Window x11parent() const { return hParent; }

                public:
                    /** Handle X11 event
                     *
                     * @param ev event to handle
                     * @return status of operation
                     */
                    virtual status_t handle_event(const event_t *ev);

                    /** Set event handler
                     *
                     * @param handler event handler
                     */
                    inline void set_handler(IEventHandler *handler)
                    {
                        pHandler = handler;
                    }

                    /** Move window
                     *
                     * @param left left coordinate
                     * @param top top coordinate
                     * @return status of operation
                     */
                    virtual status_t move(ssize_t left, ssize_t top);

                    /** Resize window
                     *
                     * @param width window width
                     * @param height window height
                     * @return status of operation
                     */
                    virtual status_t resize(ssize_t width, ssize_t height);

                    /** Set window geometry
                     *
                     * @param realize window realization structure
                     * @return status of operation
                     */
                    virtual status_t set_geometry(const rectangle_t *realize);

                    /** Set window's border style
                     *
                     * @param style window's border style
                     * @return status of operation
                     */
                    virtual status_t set_border_style(border_style_t style);

                    /** Get window's border style
                     *
                     * @param style window's border style
                     * @return status of operation
                     */
                    virtual status_t get_border_style(border_style_t *style);

                    /** Get window geometry
                     *
                     * @return window geometry
                     */
                    virtual status_t get_geometry(rectangle_t *realize);

                    virtual status_t get_absolute_geometry(rectangle_t *realize);

                    /** Hide window
                     *
                     * @return status of operation
                     */
                    virtual status_t hide();

                    /** Show window
                     *
                     * @return status of operation
                     */
                    virtual status_t show();

                    virtual status_t show(IWindow *over);

                    /**
                     * Grab events from the screen
                     * @param group grab group
                     * @return status of operation
                     */
                    virtual status_t grab_events(grab_t group);

                    /**
                     * Ungrab currently selected group of events
                     * @return status of operation
                     */
                    virtual status_t ungrab_events();

                    /** Set left coordinate of the window
                     *
                     * @param left left coordinate of the window
                     * @return status of operation
                     */
                    virtual status_t set_left(ssize_t left);

                    /** Set top coordinate of the window
                     *
                     * @param top top coordinate of the window
                     * @return status of operation
                     */
                    virtual status_t set_top(ssize_t top);

                    /** Set width of the window
                     *
                     * @param width width of the window
                     * @return status of operation
                     */
                    virtual ssize_t set_width(ssize_t width);

                    /** Set height of the window
                     *
                     * @param height height of the window
                     * @return status of operation
                     */
                    virtual ssize_t set_height(ssize_t height);

                    /** Set size constraints
                     *
                     * @param c size constraints
                     * @return status of operations
                     */
                    virtual status_t set_size_constraints(const size_limit_t *c);

                    /** Get size constraints
                     *
                     * @param c size constraints
                     * @return status of operation
                     */
                    virtual status_t get_size_constraints(size_limit_t *c);

                    /** Check constraints
                     *
                     * @return status of operation
                     */
                    virtual status_t check_constraints();

                    /** Set focus
                     *
                     * @param focus set/unset focus flag
                     * @return status of operation
                     */
                    virtual status_t set_focus(bool focus);

                    /** Toggle focus state
                     *
                     * @return status of operation
                     */
                    virtual status_t toggle_focus();

                    /** Get caption of the window
                     *
                     * @param text buffer to store data
                     * @param len length of bufer
                     * @return status of operation
                     */
                    virtual status_t get_caption(char *text, size_t len);

                    /** Set window icon
                     *
                     * @param bgra picture data in BGRA format
                     * @param width picture width
                     * @param height picture height
                     * @return status of operation
                     */
                    virtual status_t set_icon(const void *bgra, size_t width, size_t height);

                    /** Get bitmask of allowed window actions
                     *
                     * @param actions pointer to store action mask
                     * @return status of operation
                     */
                    virtual status_t get_window_actions(size_t *actions);

                    /** Set bitmask of allowed window actions
                     *
                     * @param actions action mask
                     * @return status of operation
                     */
                    virtual status_t set_window_actions(size_t actions);

                    /** Set mouse pointer
                     *
                     * @param ponter mouse pointer
                     * @return status of operation
                     */
                    virtual status_t set_mouse_pointer(mouse_pointer_t ponter);

                    /** Get mouse pointer
                     *
                     * @return mouse pointer
                     */
                    virtual mouse_pointer_t get_mouse_pointer();

                    /**
                     * Set window class
                     * @param instance window instance, ASCII-string
                     * @param wclass window class, ASCII-string
                     * @return status of operation
                     */
                    virtual status_t set_class(const char *instance, const char *wclass);

                    /**
                     * Set window role
                     * @param wrole window role, ASCII-string
                     * @return status of operation
                     */
                    virtual status_t set_role(const char *wrole);

                    virtual bool has_parent() const;
            };
        }
    
    } /* namespace x11ui */
} /* namespace lsp */

#endif /* UI_X11_WINDOW_H_ */
