/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *           (C) 2025 Marvin Edeler <marvin.edeler@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 9 June 2025
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

#ifndef PRIVATE_COCOA_COCOAWINDOW_H_
#define PRIVATE_COCOA_COCOAWINDOW_H_

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#include <vector>
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/cocoa/CocoaCairoView.h>

#include <cairo.h>
#include <cairo-quartz.h>


namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            class CocoaDisplay;

            class LSP_HIDDEN_MODIFIER CocoaWindow: public IWindow, public IEventHandler
            {
                private:
                    friend class CocoaDisplay;
                    NSApplication       *pCocoaApplication;
                    NSWindow            *pCocoaWindow;
                    CocoaCairoView      *pCocoaView;                    // The View of the window
                    NSCursor            *pCocoaCursor;                  // The Cursor of the View
                    NSWindow            *transientParent;
                    void                place_above(NSWindow *parent);
                    std::vector<id>     windowObserverTokens; 
                    std::vector<id>     viewObserverTokens; 
                
                protected:
                    typedef struct btn_event_t
                    {
                        event_t         sDown;
                        event_t         sUp;
                    } btn_event_t;

                             
                    CocoaDisplay                   *pCocoaDisplay;      // Pointer to the display
                    mouse_pointer_t                 enPointer;          // Mouse pointer
                    rectangle_t                     sSize;              // Size of the window
                    size_limit_t                    sConstraints;       // Window constraints
                    border_style_t                  enBorderStyle;      // Border style of the window
                    size_t                          nActions;           // Allowed window actions
                    window_state_t                  enState;            // Window state
                    ISurface                        *pSurface;          // Drawing surface
                    CGContextRef                    testSurface;
                    bool                            bMouseInside;       // Flag that indicates that mouse is inside of the window
                    bool                            bWrapper;           // Indicates that window is a wrapper
                    bool                            bVisible;           // Indicates that window is visible
                    bool                            bInvalidate;        // Indicates that window is invalidate
                    btn_event_t                     vBtnEvent[3];   // Events for detecting double click and triple click
                    
                    status_t                        set_geometry_impl();
                    void                            apply_constraints(rectangle_t *dst, const rectangle_t *req);
                    status_t                        commit_border_style(border_style_t bs, size_t wa);
                    void                            drop_surface();
                    cairo_surface_t                *get_image_surface();
                    static bool                     check_click(const btn_event_t *ev);
                    static bool                     check_double_click(const btn_event_t *pe, const btn_event_t *ce);
                    ISurface                       *create_surface(CocoaDisplay *display, CocoaCairoView *view, size_t width, size_t height);
                    void                            init_notification_center(NSWindow *window);               //Creates Events UIE_SHOW / UIE_HIDE
                    void                            init_notification_center(CocoaCairoView *view);           // Creates Events UIE_SHOW / UIE_HIDE
                    NSWindowStyleMask               get_ns_style(border_style_t style, size_t wa);      // Maps the border_style_t and actions to NSWindowStyleMask

                public:
                    NSWindow *nswindow() const;

                    explicit CocoaWindow(CocoaDisplay *dpy, NSView *view, IEventHandler *handler, bool wrapper);
                    virtual ~CocoaWindow();

                    virtual status_t    init() override;
                    virtual void        destroy() override;

                    virtual status_t    set_mouse_pointer(mouse_pointer_t pointer) override;
                    virtual mouse_pointer_t get_mouse_pointer() override; 

                    virtual status_t    set_caption(const char *caption) override;
                    virtual status_t    set_caption(const LSPString *caption) override;
                    virtual status_t    get_caption(char *text, size_t len) override;
                    virtual status_t    get_caption(LSPString *text) override;

                    virtual status_t    set_border_style(border_style_t style) override;
                    virtual status_t    get_border_style(border_style_t *style) override;
                    virtual status_t    resize(ssize_t width, ssize_t height) override;
                    virtual status_t    get_geometry(rectangle_t *realize) override;
                    virtual status_t    set_geometry(const rectangle_t *realize) override;
                    virtual status_t    get_absolute_geometry(rectangle_t *realize) override;

                    virtual status_t    show() override;
                    virtual status_t    show(IWindow *over) override;
                    virtual status_t    hide() override;
                    virtual bool        is_visible() override;

                    virtual status_t    get_window_actions(size_t *actions) override;
                    virtual status_t    set_window_actions(size_t actions) override;

                    virtual status_t    handle_event(const event_t *ev) override;
                    virtual ISurface   *get_surface() override;

                    virtual status_t    invalidate() override;

                    virtual status_t    set_size_constraints(const size_limit_t *c) override;
                    virtual status_t    get_size_constraints(size_limit_t *c) override;
                    
                    virtual ssize_t     left() override;
                    virtual ssize_t     top() override;
                    virtual ssize_t     width() override;
                    virtual ssize_t     height() override;

                    virtual void       *handle() override;
                    NSWindow           *get_window_handler();
                /*
                public:
                    virtual ISurface   *get_surface() override;
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

                    virtual void       *parent() const override;
                    virtual status_t    set_parent(void *parent) override;

                    virtual status_t    get_window_state(window_state_t *state) override;
                    virtual status_t    set_window_state(window_state_t state) override;

                public:
                    virtual status_t    handle_event(const event_t *ev) override;
                */

            };
        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_MACOSX */

#endif /* COCOADISPLAY_H */