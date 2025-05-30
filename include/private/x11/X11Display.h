/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef UI_X11_X11DISPLAY_H_
#define UI_X11_X11DISPLAY_H_

#include <lsp-plug.in/ws/version.h>

#ifdef USE_LIBX11

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/ws/IDisplay.h>

#include <time.h>
#include <X11/Xlib.h>

// Cairo headers
#ifdef USE_LIBCAIRO
    #include <cairo/cairo.h>
#endif /* USE_LIBCAIRO */

#include <private/x11/X11Atoms.h>
#include <private/x11/X11Window.h>
#include <private/freetype/FontManager.h>

namespace lsp
{
    namespace ws
    {
        namespace ft
        {
            class FontManager;
        }

        namespace x11
        {
            class X11Window;

        #ifdef USE_LIBCAIRO
            class X11CairoSurface;
        #endif /* USE_LIBCAIRO */

            class LSP_HIDDEN_MODIFIER X11Display: public IDisplay
            {
                friend class X11Window;

            #ifdef USE_LIBCAIRO
                friend class X11CairoSurface;
            #endif /* USE_LIBCAIRO */

                protected:
                    enum x11_async_types
                    {
                        X11ASYNC_CB_RECV,
                        X11ASYNC_CB_SEND,
                        X11ASYNC_DND_RECV,
                        X11ASYNC_DND_PROXY
                    };

                    enum x11_cb_recv_states
                    {
                        CB_RECV_CTYPE,
                        CB_RECV_SIMPLE,
                        CB_RECV_INCR
                    };

                    enum x11_dnd_recv_states
                    {
                        DND_RECV_PENDING,
                        DND_RECV_POSITION,
                        DND_RECV_ACCEPT,
                        DND_RECV_REJECT,
                        DND_RECV_SIMPLE,
                        DND_RECV_INCR
                    };

                    typedef struct wnd_lock_t
                    {
                        X11Window      *pOwner;
                        X11Window      *pWaiter;
                        ssize_t         nCounter;
                    } wnd_lock_t;

                    struct x11_async_t;

                    typedef struct cb_common_t
                    {
                        bool                bComplete;
                        Atom                hProperty;
                    } cb_common_t;

                    typedef struct cb_recv_t: public cb_common_t
                    {
                        Atom                hSelection;
                        Atom                hType;
                        x11_cb_recv_states  enState;
                        IDataSink          *pSink;
                    } cb_recv_t;

                    typedef struct cb_send_t: public cb_common_t
                    {
                        Atom                hSelection;
                        Atom                hType;
                        Window              hRequestor;
                        IDataSource        *pSource;
                        io::IInStream      *pStream;
                    } cb_send_t;

                    typedef struct dnd_recv_t: public cb_common_t
                    {
                        Window              hTarget;
                        Window              hSource;
                        Atom                hSelection;
                        Atom                hType;
                        x11_dnd_recv_states enState;
                        bool                bPollActive;
                        IDataSink          *pSink;
                        Atom                hAction;
                        Window              hProxy;
                    } dnd_recv_t;

                    typedef struct xtranslate_t
                    {
                        Window              hSrcW;          // Source Window
                        Window              hDstW;          // Destination window
                        bool                bSuccess;       // Success flag
                    } xtranslate_t;

                    typedef struct xsetinputfocus_t
                    {
                        Window              hWnd;           // Focused Window
                        bool                bSuccess;       // Success flag
                    } xsetinputfocus_t;

                    typedef struct dnd_proxy_t: public cb_common_t
                    {
                        Window              hTarget;        // The target window which has XDndProxy attribute
                        Window              hSource;        // The source window
                        Window              hCurrent;       // The current window that receives proxy events
                        long                enter[4];       // XDndEnter headers
                    } dnd_proxy_t;

                    typedef struct x11_async_t
                    {
                        x11_async_types     type;
                        status_t            result;
                        union
                        {
                            cb_common_t         cb_common;
                            cb_recv_t           cb_recv;
                            cb_send_t           cb_send;
                            dnd_recv_t          dnd_recv;
                            dnd_proxy_t         dnd_proxy;
                        };
                    } x11_async_t;

                    typedef struct x11_screen_t
                    {
                        size_t              id;
                        size_t              grabs;          // Number of grab owners
                        size_t              width;          // Width of display
                        size_t              height;         // Height of display
                        size_t              mm_width;       // Width of display in mm
                        size_t              mm_height;      // Height of display in mm
                    } x11_screen_t;

                private:
                    static atomic_t             hLock;
                    static X11Display          *pHandlers;
                    X11Display                 *pNextHandler;

                    static int                  x11_error_handler(Display *dpy, XErrorEvent *ev);

                protected:
                    volatile bool               bExit;
                    Display                    *pDisplay;
                    Window                      hRootWnd;           // Root window of the display
                    Window                      hClipWnd;           // Unmapped clipboard window
                    X11Window                  *pFocusWindow;       // Focus window after show
                    Atom                        nWakeupMessage;     // Wakeup message type
                    int                         nBlackColor;
                    int                         nWhiteColor;
                    x11_atoms_t                 sAtoms;
                    Cursor                      vCursors[__MP_COUNT];
                    size_t                      nIOBufSize;
                    uint8_t                    *pIOBuf;
                    FT_Library                  hFtLibrary;
                    IDataSource                *pCbOwner[_CBUF_TOTAL];
                #ifdef USE_LIBFREETYPE
                    ft::FontManager             sFontManager;
                #endif /* USE_LIBFREETYPE */

                    lltl::darray<x11_screen_t>  vScreens;
                    lltl::parray<X11Window>     vWindows;
                    lltl::parray<X11Window>     vGrab[__GRAB_TOTAL];
                    lltl::parray<X11Window>     sTargets;
                    lltl::darray<wnd_lock_t>    sLocks;
                    lltl::darray<x11_async_t>   sAsync;
                    lltl::parray<char>          vDndMimeTypes;
                    xtranslate_t                sTranslateReq;
                    xsetinputfocus_t            sSetInputFocusReq;

                    lltl::darray<MonitorInfo>   vMonitors;

                    ISurface                   *pEstimation;        // Estimation surface

                protected:
                    void            decode_event(event_t *ue, XEvent *ev);
                    void            handle_event(XEvent *ev);
                    bool            handle_clipboard_event(XEvent *ev);
                    bool            handle_drag_event(XEvent *ev);

                    status_t        do_main_iteration(timestamp_t ts);
                    void            do_destroy();
                    X11Window      *get_locked(X11Window *wnd);
                    X11Window      *get_redirect(X11Window *wnd);
                    static void     compress_long_data(void *data, size_t nitems);
                    Atom            gen_selection_id();
                    X11Window      *find_window(Window wnd);
                    status_t        bufid_to_atom(size_t bufid, Atom *atom);
                    status_t        atom_to_bufid(Atom x, size_t *bufid);

                    status_t        read_property(Window wnd, Atom property, Atom ptype, uint8_t **data, size_t *size, Atom *type);
                    status_t        decode_mime_types(lltl::parray<char> *ctype, const uint8_t *data, size_t size);
                    void            drop_mime_types(lltl::parray<char> *ctype);
                    static status_t sink_data_source(IDataSink *dst, IDataSource *src);

                    bool            handle_property_notify(XPropertyEvent *ev);
                    status_t        handle_property_notify(cb_recv_t *task, XPropertyEvent *ev);
                    status_t        handle_property_notify(cb_send_t *task, XPropertyEvent *ev);
                    status_t        handle_property_notify(dnd_recv_t *task, XPropertyEvent *ev);

                    void            handle_selection_notify(XSelectionEvent *ev);
                    status_t        handle_selection_notify(cb_recv_t *task, XSelectionEvent *ev);
                    status_t        handle_selection_notify(dnd_recv_t *task, XSelectionEvent *ev);

                    void            handle_selection_request(XSelectionRequestEvent *ev);
                    status_t        handle_selection_request(cb_send_t *task, XSelectionRequestEvent *ev);

                    void            handle_selection_clear(XSelectionClearEvent *ev);

                    status_t        handle_drag_enter(XClientMessageEvent *ev);
                    status_t        handle_drag_leave(dnd_recv_t *task, XClientMessageEvent *ev);
                    status_t        handle_drag_position(dnd_recv_t *task, XClientMessageEvent *ev);
                    status_t        handle_drag_drop(dnd_recv_t *task, XClientMessageEvent *ev);
                    void            complete_dnd_transfer(dnd_recv_t *task, bool success);
                    void            reject_dnd_transfer(dnd_recv_t *task);

                    void            send_immediate(Window wnd, Bool propagate, long event_mask, XEvent *event);

                    x11_async_t    *find_dnd_proxy_task(Window wnd);
                    status_t        proxy_drag_leave(dnd_proxy_t *task, XClientMessageEvent *ev);
                    status_t        proxy_drag_position(dnd_proxy_t *task, XClientMessageEvent *ev);
                    status_t        proxy_drag_drop(dnd_proxy_t *task, XClientMessageEvent *ev);

                    x11_async_t    *lookup_dnd_proxy_task();

                    dnd_recv_t     *current_drag_task();
                    void            complete_async_tasks();

                    status_t        init_freetype_library();

                    status_t        read_window_state(window_state_t *state, Window wnd);

                    bool            translate_coordinates(Window src_w, Window dest_w, int src_x, int src_y, int *dest_x, int *dest_y, Window *child_return);

                    static void                 drop_monitors(lltl::darray<MonitorInfo> *list);

                protected:
                    virtual bool                r3d_backend_supported(const r3d::backend_metadata_t *meta) override;
                    virtual void                task_queue_changed() override;

                public:
                    explicit X11Display();
                    virtual ~X11Display();

                    virtual status_t            init(int argc, const char **argv) override;
                    virtual void                destroy() override;

                public:
                    virtual IWindow            *create_window() override;
                    virtual IWindow            *create_window(size_t screen) override;
                    virtual IWindow            *create_window(void *handle) override;
                    virtual IWindow            *wrap_window(void *handle) override;

                    virtual status_t            main() override;
                    virtual status_t            main_iteration() override;
                    virtual void                quit_main() override;
                    virtual status_t            wait_events(wssize_t millis) override;

                    virtual size_t              screens() override;
                    virtual size_t              default_screen() override;
                    virtual status_t            screen_size(size_t screen, ssize_t *w, ssize_t *h) override;
                    virtual status_t            work_area_geometry(ws::rectangle_t *r) override;

                    virtual status_t            set_clipboard(size_t id, IDataSource *ds) override;
                    virtual status_t            get_clipboard(size_t id, IDataSink *dst) override;

                    virtual const char * const *get_drag_ctypes() override;
                    virtual status_t            reject_drag() override;
                    virtual status_t            accept_drag(IDataSink *sink, drag_t action, const rectangle_t *r = NULL) override;
                    virtual bool                drag_pending() override;

                    virtual status_t            get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top) override;

                    virtual status_t            add_font(const char *name, io::IInStream *is) override;
                    virtual status_t            add_font_alias(const char *name, const char *alias) override;
                    virtual status_t            remove_font(const char *name) override;
                    virtual void                remove_all_fonts() override;

                    virtual bool                get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool                get_text_parameters(const Font &f, text_parameters_t *tp, const char *text) override;
                    virtual bool                get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual const MonitorInfo  *enum_monitors(size_t *count) override;

                    virtual void                sync() override;

                public:
                    void                        handle_error(XErrorEvent *ev);

                    bool                        add_window(X11Window *wnd);
                    bool                        remove_window(X11Window *wnd);

                    inline Display             *x11display() const  { return pDisplay; }
                    inline Window               x11root() const     { return hRootWnd; }
                    inline const x11_atoms_t   &atoms() const       { return sAtoms; }
                    Cursor                      get_cursor(mouse_pointer_t pointer);

                    size_t                      get_screen(Window root);

                    status_t                    grab_events(X11Window *wnd, grab_t group);
                    status_t                    ungrab_events(X11Window *wnd);

                    status_t                    lock_events(X11Window *wnd, X11Window *lock);
                    status_t                    unlock_events(X11Window *wnd);

                    ft::FontManager            *font_manager();

                    bool                        set_input_focus(::Window wnd);

                    void                        flush();

                public:
                    static const char          *event_name(int xev_code);
            };
        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */

#endif /* UI_X11_X11DISPLAY_H_ */
