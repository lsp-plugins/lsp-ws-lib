/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 дек. 2016 г.
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

#ifndef LSP_PLUG_IN_WS_IWINDOW_H_
#define LSP_PLUG_IN_WS_IWINDOW_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IEventHandler.h>

#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace ws
    {
        class IDisplay;

        /** Native window class
         *
         */
        class LSP_WS_LIB_PUBLIC IWindow
        {
            protected:
                IEventHandler  *pHandler;
                IDisplay       *pDisplay;

            private:
                IWindow & operator = (const IWindow);
                IWindow(const IWindow &);

            public:
                explicit IWindow(IDisplay *dpy, IEventHandler *handler = NULL);
                virtual ~IWindow();

                /** Window initialization routine
                 *
                 * @return status of operation
                 */
                virtual status_t init();

                /** Window finalization routine
                 *
                 */
                virtual void destroy();

            public:
                /**
                 * Get display
                 * @return display
                 */
                inline IDisplay *display()          { return pDisplay; }

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

                /**
                 * Invalidate contents of the window and issue it's redraw.
                 * @return status of operation
                 */
                virtual status_t invalidate();

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

                /** Return current screen of the window
                 *
                 * @return screen of the window
                 */
                virtual size_t screen();

                /** Set caption of the window
                 *
                 * @param caption UTF-8-encoded NULL-terminated caption string
                 * @return status of operation
                 */
                virtual status_t set_caption(const char *caption);
                virtual status_t set_caption(const LSPString *caption);

            public:
                /** Get native handle of the window
                 *
                 */
                virtual void *handle();

                /** Set event handler
                 *
                 * @param handler event handler
                 */
                inline void set_handler(IEventHandler *handler)     { pHandler = handler; }

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
                 * @param left left coordinate
                 * @param top top coordinate
                 * @param width window width
                 * @param height window height
                 * @return status of operation
                 */
                virtual status_t set_geometry(ssize_t left, ssize_t top, ssize_t width, ssize_t height);

                /** Set window geometry
                 *
                 * @param size actual window parameters
                 * @return status of operation
                 */
                virtual status_t set_geometry(const rectangle_t *size);

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

                /** Get window geometry. Obtains the size of the client area
                 * of the window and it's location relative to the parent window.
                 *
                 * @return window geometry
                 */
                virtual status_t get_geometry(rectangle_t *size);

                /** Get absolute window's geometry. Obtains the size of the whole window
                 * and it's location relative to the screen.
                 *
                 * @param size pointer to structure to store data
                 * @return status of operation
                 */
                virtual status_t get_absolute_geometry(rectangle_t *size);

                /** Get caption
                 *
                 * @param text pointer to store data (buffer for UTF-8 string)
                 * @param len number of characters
                 * @return status of operation
                 */
                virtual status_t get_caption(char *text, size_t len);

                /** Get caption
                 *
                 * @param text pointer to store data (buffer for UTF-8 string)
                 * @return status of operation
                 */
                virtual status_t get_caption(LSPString *text);

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

                /** Show window over another
                 *
                 * @param over the underlying window
                 * @return status of operation
                 */
                virtual status_t show(IWindow *over);

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

                /** Set window visibility
                 *
                 * @param visible visibility
                 * @return status of operation
                 */
                virtual status_t set_visibility(bool visible);

                /** Set size constraints of the window
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

                /** Set size constraints
                 *
                 * @param min_width minimum width
                 * @param min_height minimum height
                 * @param max_width maximum width
                 * @param max_height maximum height
                 * @return status of operation
                 */
                virtual status_t set_size_constraints(ssize_t min_width, ssize_t min_height, ssize_t max_width, ssize_t max_height);

                /** Set minimum width
                 *
                 * @param value minimum width
                 * @return status of operation
                 */
                virtual status_t set_min_width(ssize_t value);

                /** Set minimum height
                 *
                 * @param value minimum height
                 * @return status of operation
                 */
                virtual status_t set_min_height(ssize_t value);

                /** Set maximum width
                 *
                 * @param value maximum width
                 * @return status of operation
                 */
                virtual status_t set_max_width(ssize_t value);

                /** Set maximum height
                 *
                 * @param value maximum height
                 * @return status of operation
                 */
                virtual status_t set_max_height(ssize_t value);

                /** Set minimum size
                 *
                 * @param width minimum width
                 * @param height minimum height
                 * @return status of operation
                 */
                virtual status_t set_min_size(ssize_t width, ssize_t height);

                /** Set maximum size
                 *
                 * @param width maximum width
                 * @param height maximum height
                 * @return status of operation
                 */
                virtual status_t set_max_size(ssize_t width, ssize_t height);

                /** Take the focus by window
                 * @return status of operation
                 */
                virtual status_t take_focus();

                /** Set window icon
                 *
                 * @param bgra the packed array of Blue, Green, Red and Alpha components
                 * @param width width of the image
                 * @param height height of the image
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
                virtual status_t set_mouse_pointer(mouse_pointer_t pointer);

                /** Get mouse pointer
                 *
                 * @return mouse pointer
                 */
                virtual mouse_pointer_t get_mouse_pointer();

                /**
                 * Grab mouse and keyboard events for the specified group.
                 * Group declares the set of controls that will receive events only
                 * when there are no windows with grabbing events in more prioretized group.
                 *
                 * @param group the group of events to grab
                 * @return status of operation
                 */
                virtual status_t grab_events(grab_t group);

                /**
                 * Release grab of events
                 * @return status of operation
                 */
                virtual status_t ungrab_events();

                /**
                 * Check whether window is currently grabbing events
                 * @return true if window is currently grabbing events
                 */
                virtual bool is_grabbing_events() const;

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

                /**
                 * Check that window is embedded into some another window (has parents).
                 * @return true if window is embedded into some another window.
                 */
                virtual bool has_parent() const;

                /**
                 * Get handle to the parent window
                 * @return native handle of the parent window or NULL
                 */
                virtual void *parent() const;

                /**
                 * Set new parent window
                 * @param parent parent window to set or NULL if there is no need in the parent window
                 * @return status of operation
                 */
                virtual status_t set_parent(void *parent);

                /**
                 * Get window state
                 * @param state pointer to store window state
                 * @return status of operation
                 */
                virtual status_t get_window_state(window_state_t *state);

                /**
                 * Set window state
                 * @param state window state
                 */
                virtual status_t set_window_state(window_state_t state);
        };
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IWINDOW_H_ */
