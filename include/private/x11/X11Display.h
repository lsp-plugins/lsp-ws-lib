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

#ifndef UI_X11_X11DISPLAY_H_
#define UI_X11_X11DISPLAY_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/ws/IDisplay.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/pphash.h>

#include <private/x11/X11Atoms.h>
#include <private/x11/X11Window.h>

#include <time.h>
#include <X11/Xlib.h>

// Freetype headers
#ifdef USE_LIBFREETYPE
    #include <ft2build.h>
    #include FT_SFNT_NAMES_H
    #include FT_FREETYPE_H
    #include FT_GLYPH_H
    #include FT_OUTLINE_H
    #include FT_BBOX_H
    #include FT_TYPE1_TABLES_H
#endif /* USE_LIBFREETYPE */

// Cairo headers
#ifdef USE_LIBCAIRO
    #include <cairo/cairo.h>
#endif /* USE_LIBCAIRO */

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class X11Window;

        #ifdef USE_LIBCAIRO
            class X11CairoSurface;
        #endif /* USE_LIBCAIRO */

            class X11Display: public IDisplay
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

                public:
                    typedef struct font_t
                    {
                        char               *name;           // Name of the font
                        char               *alias;          // Font alias (the symbolic name of the font)
                        void               *data;           // Font data (font file contents loaded to memory)
                        ssize_t             refs;           // Number of references
                        FT_Face             ft_face;        // Font face handle for freetype

                    #ifdef USE_LIBCAIRO
                        cairo_font_face_t  *cr_face[4];     // Font faces for cairo
                    #endif /* USE_LIBCAIRO */
                    } font_t;

                private:
                    static volatile atomic_t    hLock;
                    static X11Display          *pHandlers;
                    X11Display                 *pNextHandler;

                    static int                  x11_error_handler(Display *dpy, XErrorEvent *ev);

                protected:
                    volatile bool               bExit;
                    Display                    *pDisplay;
                    Window                      hRootWnd;           // Root window of the display
                    Window                      hClipWnd;           // Unmapped clipboard window
                    X11Window                  *pFocusWindow;       // Focus window after show
                    int                         nBlackColor;
                    int                         nWhiteColor;
                    x11_atoms_t                 sAtoms;
                    Cursor                      vCursors[__MP_COUNT];
                    size_t                      nIOBufSize;
                    uint8_t                    *pIOBuf;
                    FT_Library                  hFtLibrary;
                    IDataSource                *pCbOwner[_CBUF_TOTAL];
                #ifdef USE_LIBCAIRO
                    cairo_user_data_key_t       sCairoUserDataKey;
                #endif /* USE_LIBCAIRO */

                    lltl::darray<dtask_t>       sPending;
                    lltl::darray<x11_screen_t>  vScreens;
                    lltl::parray<X11Window>     vWindows;
                    lltl::parray<X11Window>     vGrab[__GRAB_TOTAL];
                    lltl::parray<X11Window>     sTargets;
                    lltl::darray<wnd_lock_t>    sLocks;
                    lltl::darray<x11_async_t>   sAsync;
                    lltl::parray<char>          vDndMimeTypes;
                    lltl::pphash<char, font_t>  vCustomFonts;
                    xtranslate_t                sTranslateReq;

                    lltl::darray<MonitorInfo>   vMonitors;

                protected:
                    void            handle_event(XEvent *ev);
                    bool            handle_clipboard_event(XEvent *ev);
                    bool            handle_drag_event(XEvent *ev);
                    static void     destroy_font_object(font_t *font);
                    static void     unload_font_object(font_t *font);
                    static font_t  *alloc_font_object(const char *name);

                    status_t        do_main_iteration(timestamp_t ts);
                    void            do_destroy();
                    void            drop_custom_fonts();
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

                    void            handle_property_notify(XPropertyEvent *ev);
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

                    bool            translate_coordinates(Window src_w, Window dest_w, int src_x, int src_y, int *dest_x, int *dest_y, Window *child_return);

                    virtual bool                r3d_backend_supported(const r3d::backend_metadata_t *meta);

                    static void                 drop_monitors(lltl::darray<MonitorInfo> *list);

                public:
                    explicit X11Display();
                    virtual ~X11Display();

                    virtual status_t            init(int argc, const char **argv);
                    virtual void                destroy();

                public:
                    virtual IWindow            *create_window();
                    virtual IWindow            *create_window(size_t screen);
                    virtual IWindow            *create_window(void *handle);
                    virtual IWindow            *wrap_window(void *handle);
                    virtual ISurface           *create_surface(size_t width, size_t height);

                    virtual status_t            main();
                    virtual status_t            main_iteration();
                    virtual void                quit_main();
                    virtual status_t            wait_events(wssize_t millis);

                    virtual size_t              screens();
                    virtual size_t              default_screen();
                    virtual status_t            screen_size(size_t screen, ssize_t *w, ssize_t *h);

                    virtual status_t            set_clipboard(size_t id, IDataSource *ds);
                    virtual status_t            get_clipboard(size_t id, IDataSink *dst);
                    virtual const char * const *get_drag_ctypes();

                    virtual status_t            reject_drag();
                    virtual status_t            accept_drag(IDataSink *sink, drag_t action, bool internal, const rectangle_t *r);

                    void                        handle_error(XErrorEvent *ev);

                    virtual status_t            get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top);

                    virtual status_t            add_font(const char *name, const char *path);
                    virtual status_t            add_font(const char *name, const io::Path *path);
                    virtual status_t            add_font(const char *name, const LSPString *path);
                    virtual status_t            add_font(const char *name, io::IInStream *is);
                    virtual status_t            add_font_alias(const char *name, const char *alias);
                    virtual status_t            remove_font(const char *name);
                    virtual void                remove_all_fonts();

                    virtual const MonitorInfo  *enum_monitors(size_t *count);

                public:
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

                    font_t                     *get_font(const char *name);

                    virtual void                sync();
                    void                        flush();

                public:
                    static const char          *event_name(int xev_code);
            };
        }
    }
}

#endif /* UI_X11_X11DISPLAY_H_ */
