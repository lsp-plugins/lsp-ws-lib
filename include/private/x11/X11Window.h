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

#ifdef USE_LIBX11

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
                    virtual status_t    init() override;

                    /** Window finalization routine
                     *
                     */
                    virtual void        destroy() override;

                public:
                    inline ::Window x11handle() const { return hWindow; }
                    inline ::Window x11parent() const { return hParent; }

                public:
                    virtual ISurface   *get_surface() override;

                    virtual ssize_t     left() override;
                    virtual ssize_t     top() override;
                    virtual ssize_t     width() override;
                    virtual ssize_t     height() override;
                    virtual status_t    set_left(ssize_t left) override;
                    virtual status_t    set_top(ssize_t top) override;
                    virtual ssize_t     set_width(ssize_t width) override;
                    virtual ssize_t     set_height(ssize_t height) override;
                    virtual status_t    set_geometry(const rectangle_t *realize) override;
                    virtual status_t    get_geometry(rectangle_t *realize) override;
                    virtual status_t    get_absolute_geometry(rectangle_t *realize) override;

                    virtual void       *handle() override;

                    virtual size_t      screen() override;

                    virtual status_t    move(ssize_t left, ssize_t top) override;
                    virtual status_t    resize(ssize_t width, ssize_t height) override;
                    virtual status_t    set_border_style(border_style_t style) override;
                    virtual status_t    get_border_style(border_style_t *style) override;

                    virtual bool        is_visible() override;
                    virtual status_t    hide() override;
                    virtual status_t    show() override;
                    virtual status_t    show(IWindow *over) override;

                    virtual status_t    grab_events(grab_t group) override;
                    virtual status_t    ungrab_events() override;

                    virtual status_t    set_size_constraints(const size_limit_t *c) override;
                    virtual status_t    get_size_constraints(size_limit_t *c) override;
                    virtual status_t    check_constraints() override;

                    virtual status_t    set_focus(bool focus) override;
                    virtual status_t    toggle_focus() override;

                    virtual status_t    set_caption(const char *ascii, const char *utf8) override;
                    virtual status_t    get_caption(char *text, size_t len) override;

                    virtual status_t    set_icon(const void *bgra, size_t width, size_t height) override;

                    virtual status_t    get_window_actions(size_t *actions) override;
                    virtual status_t    set_window_actions(size_t actions) override;

                    virtual status_t    set_mouse_pointer(mouse_pointer_t ponter) override;
                    virtual mouse_pointer_t get_mouse_pointer() override;

                    virtual status_t    set_class(const char *instance, const char *wclass) override;
                    virtual status_t    set_role(const char *wrole) override;

                    virtual bool        has_parent() const override;

                public:
                    virtual status_t handle_event(const event_t *ev) override;
            };
        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */

#endif /* UI_X11_WINDOW_H_ */
