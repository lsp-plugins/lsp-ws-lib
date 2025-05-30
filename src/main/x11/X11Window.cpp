/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/types.h>

#ifdef USE_LIBX11

#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/x11/X11Atoms.h>
#include <private/x11/X11Window.h>
#include <private/x11/X11Display.h>
#include <private/x11/X11CairoSurface.h>
#include <private/x11/X11GLSurface.h>

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL_GLX
    #include <private/glx/defs.h>
    #include <private/gl/Surface.h>
#endif /* USE_LIBGL */

#include <limits.h>
#include <errno.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
        #ifdef LSP_PLUGINS_USE_OPENGL_GLX
            static const GLint rgba24x32[]    = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 32, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgba24x24[]    = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgba16x24[]    = { GLX_RGBA, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 6, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgba15x24[]    = { GLX_RGBA, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 5, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgba16[]       = { GLX_RGBA, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 6, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgba15[]       = { GLX_RGBA, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 5, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None };
            static const GLint rgbax32[]      = { GLX_RGBA, GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 8, None };
            static const GLint rgbax24[]      = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 8, None };
            static const GLint rgbax16[]      = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 8, None };
            static const GLint rgba[]         = { GLX_RGBA, GLX_DOUBLEBUFFER, None };

            static const GLint * const glx_visuals[] =
            {
                rgba24x32, rgba24x24,
                rgba16x24, rgba16,
                rgba15x24, rgba15,
                rgbax32, rgbax24, rgbax16, rgba,
                NULL
            };

            static ::XVisualInfo *choose_visual(Display *dpy, int screen)
            {
                for (const GLint * const *visual = glx_visuals; *visual != NULL; ++visual)
                {
                    ::XVisualInfo *vi = ::glXChooseVisual(dpy, screen, const_cast<int *>(*visual));
                    if (vi != NULL)
                    {
                        lsp_trace(
                            "Selected visual: id=0x%lx, depth=%d, red=0x%lx, green=0x%lx, blue=0x%lx, colormap_size=%d, bits_per_rgb=%d",
                            long(vi->visualid), vi->depth, vi->red_mask, vi->green_mask, vi->blue_mask, vi->colormap_size, vi->bits_per_rgb);
                        return vi;
                    }
                }
                return NULL;
            }

        #endif /* LSP_PLUGINS_USE_OPENGL_GLX */

            LSP_HIDDEN_MODIFIER
            bool check_env_option_enabled(const char *name)
            {
                LSPString var;
                status_t res = system::get_env_var(name, &var);
                if (res != STATUS_OK)
                    return true;

                if (var.equals_ascii_nocase("no") ||
                    var.equals_ascii_nocase("n") ||
                    var.equals_ascii_nocase("disabled") ||
                    var.equals_ascii_nocase("off") ||
                    var.equals_ascii_nocase("0"))
                    return false;

                return true;
            }

            static ISurface *create_surface(X11Display *dpy, int screen, Window window, Visual *visual, size_t width, size_t height)
            {
                ISurface *result = NULL;

            #ifdef LSP_PLUGINS_USE_OPENGL_GLX
                if (check_env_option_enabled("LSP_WS_LIB_GLXSURFACE"))
                {
                    gl::context_param_t cp[4];
                    cp[0].id        = gl::DISPLAY;
                    cp[0].ptr       = dpy->x11display();
                    cp[1].id        = gl::SCREEN;
                    cp[1].sint      = screen;
                    cp[2].id        = gl::WINDOW;
                    cp[2].ulong     = window;
                    cp[3].id        = gl::END;

                    gl::IContext *ctx   = gl::create_context(cp);
                    lsp_finally { safe_release(ctx); };
                    if (ctx != NULL)
                    {
                        result = new X11GLSurface(dpy, ctx, width, height);
                        if (result == NULL)
                            ctx->invalidate();
                        else
                            lsp_trace("Using X11GLSurface ptr=%p", result);
                    }
                }
            #endif /* LSP_PLUGINS_USE_OPENGL_GLX */

                if (result == NULL)
                {
                    result = new X11CairoSurface(dpy, window, visual, width, height);
                    if (result != NULL)
                        lsp_trace("Using X11CairoSurface ptr=%p", result);
                }

                return result;
            }

            X11Window::X11Window(X11Display *core, size_t screen, ::Window wnd, IEventHandler *handler, bool wrapper): IWindow(core, handler)
            {
//                lsp_trace("hwindow = %x", int(wnd));
                pX11Display             = core;
                bWrapper                = wrapper;
                bVisible                = false;
                pVisualInfo             = NULL;
                hColormap               = None;
                if (wrapper)
                {
                    hWindow                 = wnd;
                    hParent                 = None;
                }
                else
                {
                    hWindow                 = None;
                    hParent                 = wnd;
                }
                hTransientFor           = None;
                nScreen                 = screen;
                pSurface                = NULL;
                enBorderStyle           = BS_SIZEABLE;

                sMotif.flags            = 0;
                sMotif.functions        = 0;
                sMotif.decorations      = 0;
                sMotif.input_mode       = 0;
                sMotif.status           = 0;

                nActions                = WA_SINGLE;
                nFlags                  = 0;
                enPointer               = MP_DEFAULT;
                enState                 = WS_NORMAL;

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

                for (size_t i=0; i<3; ++i)
                {
                    init_event(&vBtnEvent[i].sDown);
                    init_event(&vBtnEvent[i].sUp);
                    vBtnEvent[i].sDown.nType    = UIE_UNKNOWN;
                    vBtnEvent[i].sUp.nType      = UIE_UNKNOWN;
                }
            }

            void X11Window::do_create()
            {
            }

            X11Window::~X11Window()
            {
                pX11Display       = NULL;
            }

            status_t X11Window::init()
            {
                // Register the window
                if (pX11Display == NULL)
                    return STATUS_BAD_STATE;

                // Initialize parent class
                status_t res = IWindow::init();
                if (res != STATUS_OK)
                    return res;

                Display *dpy = pX11Display->x11display();
                Atom dnd_version    = 5;    // Version 5 of protocol is supported

                if (bWrapper)
                {
                    if (!pX11Display->add_window(this))
                        return STATUS_NO_MEM;

                    // Now select input for the handle
//                    lsp_trace("Issuing XSelectInput");
                    ::XSelectInput(dpy, hWindow,
                        KeyPressMask |
                        KeyReleaseMask |
                        ButtonPressMask |
                        ButtonReleaseMask |
                        EnterWindowMask |
                        LeaveWindowMask |
                        PointerMotionMask |
        //                PointerMotionHintMask |
                        Button1MotionMask |
                        Button2MotionMask |
                        Button3MotionMask |
                        Button4MotionMask |
                        Button5MotionMask |
                        ButtonMotionMask |
                        KeymapStateMask |
                        ExposureMask |
                        StructureNotifyMask |
                        FocusChangeMask |
                        PropertyChangeMask
                    );

                    /**
                     * In order for the user to be able to transfer data from any application to any other application
                     * via DND, every application that supports XDND version N must also support all previous versions
                     * (3 to N-1). The XdndAware property provides the highest version number supported by the target
                     * (Nt). If the source supports versions up to Ns, then the version that will actually be used is
                     * min(Ns,Nt). This is the version sent in the XdndEnter message. It is important to note that
                     * XdndAware allows this to be calculated before any messages are actually sent.
                     */
                    ::XChangeProperty(dpy, hWindow, pX11Display->atoms().X11_XdndAware, XA_ATOM, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char *>(&dnd_version), 1);
                    /**
                     * The proxy window must have the XdndProxy property set to point to itself. If it doesn't or if the
                     * proxy window doesn't exist at all, one should ignore XdndProxy on the assumption that it is left
                     * over after a crash.
                     */
                    ::XChangeProperty(dpy, hWindow, pX11Display->atoms().X11_XdndProxy, XA_WINDOW, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char *>(&hWindow), 1);

                    pX11Display->flush();
                }
                else
                {
                    // Try to create window
                    pX11Display->sync();

                    // Calculate window constraints
                    calc_constraints(&sSize, &sSize);

                    // Determine parent window and screen
                    Window wnd = 0;
                    Window parent_wnd = hParent;

                    if (parent_wnd > 0)
                    {
                        XWindowAttributes atts;
                        XGetWindowAttributes(dpy, hParent, &atts);
                        nScreen = pX11Display->get_screen(atts.root);
                    }
                    else
                    {
                        const size_t n = pX11Display->screens();
                        parent_wnd = (nScreen < n) ? RootWindow(dpy, nScreen) : pX11Display->x11root();
                        nScreen = pX11Display->get_screen(wnd);
                    }

                #ifdef LSP_PLUGINS_USE_OPENGL_GLX
                    // Create visual
                    pVisualInfo = choose_visual(dpy, int(nScreen));
                    ::Visual *xv = (pVisualInfo != NULL) ? pVisualInfo->visual : ::XDefaultVisual(dpy, int(nScreen));
                    lsp_trace(
                        "Selected visual: id=0x%lx, red=0x%lx, green=0x%lx, blue=0x%lx, bits_per_rgb=%d",
                        xv->visualid, xv->red_mask, xv->green_mask, xv->blue_mask, xv->bits_per_rgb);

                    hColormap = XCreateColormap(dpy, parent_wnd, xv, AllocNone);

                    XSetWindowAttributes swa;
                    swa.colormap            = hColormap;
                    swa.background_pixmap   = None;
                    swa.border_pixel        = 0;

                    wnd = XCreateWindow(
                        dpy, parent_wnd,
                        sSize.nLeft, sSize.nTop, sSize.nWidth, sSize.nHeight,
                        0, 0, CopyFromParent, xv, CWColormap|CWBorderPixel, &swa);
                #else
                    wnd = XCreateWindow(
                        dpy, parent_wnd,
                        sSize.nLeft, sSize.nTop, sSize.nWidth, sSize.nHeight,
                        0, 0, CopyFromParent, CopyFromParent, 0, NULL);
                #endif /* LSP_PLUGINS_USE_OPENGL_GLX */

//                    lsp_trace("wnd=%x, external=%d, external_id=%x", int(wnd), int(hParent > 0), int(hParent));
                    if (wnd <= 0)
                        return STATUS_UNKNOWN_ERR;
                    pX11Display->flush();

                    // Get protocols
//                    lsp_trace("Issuing XSetWMProtocols");
                    Atom atom_close     = pX11Display->atoms().X11_WM_DELETE_WINDOW;
                    ::XSetWMProtocols(dpy, wnd, &atom_close, 1);

                    /**
                     * In order for the user to be able to transfer data from any application to any other application
                     * via DND, every application that supports XDND version N must also support all previous versions
                     * (3 to N-1). The XdndAware property provides the highest version number supported by the target
                     * (Nt). If the source supports versions up to Ns, then the version that will actually be used is
                     * min(Ns,Nt). This is the version sent in the XdndEnter message. It is important to note that
                     * XdndAware allows this to be calculated before any messages are actually sent.
                     */
                    ::XChangeProperty(dpy, wnd, pX11Display->atoms().X11_XdndAware, XA_ATOM, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char *>(&dnd_version), 1);
                    /**
                     * The proxy window must have the XdndProxy property set to point to itself. If it doesn't or if the
                     * proxy window doesn't exist at all, one should ignore XdndProxy on the assumption that it is left
                     * over after a crash.
                     */
                    ::XChangeProperty(dpy, wnd, pX11Display->atoms().X11_XdndProxy, XA_WINDOW, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char *>(&wnd), 1);
                    pX11Display->flush();

                    // Now create X11Window instance
                    if (!pX11Display->add_window(this))
                    {
                        XDestroyWindow(dpy, wnd);
                        pX11Display->flush();
                        return STATUS_NO_MEM;
                    }

                    // Now select input for new handle
//                    lsp_trace("Issuing XSelectInput");
                    ::XSelectInput(dpy, wnd,
                        KeyPressMask |
                        KeyReleaseMask |
                        ButtonPressMask |
                        ButtonReleaseMask |
                        EnterWindowMask |
                        LeaveWindowMask |
                        PointerMotionMask |
        //                PointerMotionHintMask |
                        Button1MotionMask |
                        Button2MotionMask |
                        Button3MotionMask |
                        Button4MotionMask |
                        Button5MotionMask |
                        ButtonMotionMask |
                        KeymapStateMask |
                        ExposureMask |
        //                VisibilityChangeMask |
                        StructureNotifyMask |
        //                ResizeRedirectMask |
                        SubstructureNotifyMask |
                        SubstructureRedirectMask |
                        FocusChangeMask |
                        PropertyChangeMask |
                        ColormapChangeMask |
                        OwnerGrabButtonMask
                    );
                    if (hParent > 0)
                    {
                        ::XSelectInput(dpy, hParent,
                            PropertyChangeMask |
                            StructureNotifyMask
                        );
                    }

                    pX11Display->flush();

                    sMotif.flags        = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE | MWM_HINTS_STATUS;
                    sMotif.functions    = MWM_FUNC_ALL;
                    sMotif.decorations  = MWM_DECOR_ALL;
                    sMotif.input_mode   = MWM_INPUT_MODELESS;
                    sMotif.status       = 0;

                    hWindow = wnd;

                    // Initialize window border style and actions
                    set_border_style(BS_SIZEABLE);
                    set_window_actions(WA_ALL);
                    set_mouse_pointer(MP_DEFAULT);
                }

                lsp_trace("init ok, hwindow = %x", int(hWindow));

                return STATUS_OK;
            }

            void X11Window::drop_surface()
            {
                if (pSurface != NULL)
                {
                    pSurface->destroy();
                    delete pSurface;
                    pSurface = NULL;
                }
            }

            void X11Window::destroy()
            {
                lsp_trace("hWindow=0x%lx", hWindow);

                // Drop surface
                hide();
                drop_surface();

                if (!bWrapper)
                {
                    // Remove window from registry
                    if (pX11Display != NULL)
                        pX11Display->remove_window(this);

                    // Destroy window
                    if (hWindow > 0)
                    {
                        XDestroyWindow(pX11Display->x11display(), hWindow);
                        hWindow = 0;
                    }
                    pX11Display->sync();
                }
                else
                {
                    hWindow = None;
                    hParent = None;
                }

                if (pX11Display != NULL)
                {
                    if (hColormap != None)
                    {
                        XFreeColormap(pX11Display->x11display(), hColormap);
                        hColormap       = None;
                    }
                    if (pVisualInfo != NULL)
                    {
                        XFree(pVisualInfo);
                        pVisualInfo     = NULL;
                    }
                }

                pX11Display = NULL;
                IWindow::destroy();
            }

            void X11Window::calc_constraints(rectangle_t *dst, const rectangle_t *req)
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

            ISurface *X11Window::get_surface()
            {
                if (bWrapper)
                    return NULL;
                return pSurface;
            }

            status_t X11Window::invalidate()
            {
                if ((pSurface == NULL) || (hWindow == None))
                    return STATUS_BAD_STATE;

                XEvent ev;
                XExposeEvent *ex = &ev.xexpose;

                ex->type        = Expose;
                ex->serial      = 0;
                ex->send_event  = True;
                ex->display     = NULL;
                ex->window      = hWindow;
                ex->x           = sSize.nLeft;
                ex->y           = sSize.nHeight;
                ex->width       = sSize.nWidth;
                ex->height      = sSize.nHeight;
                ex->count       = 0;

                ::XSendEvent(pX11Display->x11display(), hWindow, False, NoEventMask, &ev);
                pX11Display->flush();
                return STATUS_OK;
            }

            status_t X11Window::do_update_constraints(bool disable)
            {
                if (hWindow == 0)
                    return STATUS_BAD_STATE;

                XSizeHints sz;
                sz.flags        = USPosition | USSize | PMinSize | PMaxSize;
                sz.x            = sSize.nLeft;
                sz.y            = sSize.nTop;
                sz.width        = sSize.nWidth;
                sz.height       = sSize.nHeight;

                if (disable)
                {
                    sz.min_width    = 1;
                    sz.min_height   = 1;
                    sz.max_width    = INT_MAX;
                    sz.max_height   = INT_MAX;
                }
                else if (nActions & WA_RESIZE)
                {
                    sz.min_width    = (sConstraints.nMinWidth > 0) ? sConstraints.nMinWidth : 1;
                    sz.min_height   = (sConstraints.nMinHeight > 0) ? sConstraints.nMinHeight : 1;
                    sz.max_width    = (sConstraints.nMaxWidth > 0) ? sConstraints.nMaxWidth : INT_MAX;
                    sz.max_height   = (sConstraints.nMaxHeight > 0) ? sConstraints.nMaxHeight : INT_MAX;
                }
                else
                {
                    sz.min_width    = sSize.nWidth;
                    sz.min_height   = sSize.nHeight;
                    sz.max_width    = sSize.nWidth;
                    sz.max_height   = sSize.nHeight;
                }

//                lsp_trace("Window constraints: pos=(x=%d, y=%d), size=(w=%d, h=%d), min(w=%d, h=%d), max(w=%d, height=%d)",
//                    int(sz.x), int(sz.y), int(sz.width), int(sz.height),
//                    int(sz.min_width), int(sz.min_height), int(sz.max_width), int(sz.max_height)
//                );

                XSetWMNormalHints(pX11Display->x11display(), hWindow, &sz);
//                pX11Display->sync();
                return STATUS_OK;
            }

            bool X11Window::check_click(const btn_event_t *ev)
            {
                if ((ev->sDown.nType != UIE_MOUSE_DOWN) || (ev->sUp.nType != UIE_MOUSE_UP))
                    return false;
                if (ev->sDown.nCode != ev->sUp.nCode)
                    return false;
                if ((ev->sUp.nTime < ev->sDown.nTime) || ((ev->sUp.nTime - ev->sDown.nTime) > 400))
                    return false;

                return (ev->sDown.nLeft == ev->sUp.nLeft) && (ev->sDown.nTop == ev->sUp.nTop);
            }

            bool X11Window::check_double_click(const btn_event_t *pe, const btn_event_t *ev)
            {
                if (!check_click(pe))
                    return false;

                if (pe->sDown.nCode != ev->sDown.nCode)
                    return false;
                if ((ev->sUp.nTime < pe->sUp.nTime) || ((ev->sUp.nTime - pe->sUp.nTime) > 400))
                    return false;

                return (pe->sUp.nLeft == ev->sUp.nLeft) && (pe->sUp.nTop == ev->sUp.nTop);
            }

            status_t X11Window::handle_event(const event_t *ev)
            {
                // Additionally generated event
                event_t gen;
                gen.nType       = UIE_UNKNOWN;
                IEventHandler *handler = pHandler;
    //            lsp_trace("ui_event type=%d", int(ev->nType));

                switch (ev->nType)
                {
                    case UIE_SHOW:
                    {
                        bVisible        = true;
                        if (bWrapper)
                            break;

                        // Drop previously existed drawing surface
                        drop_surface();

                        // Create surface
                        Display *dpy    = pX11Display->x11display();
                        ::Visual *v     = (pVisualInfo != NULL) ? pVisualInfo->visual : DefaultVisual(dpy, screen());
                        pSurface        = create_surface(
                            static_cast<X11Display *>(pDisplay), int(screen()), hWindow, v,
                            sSize.nWidth, sSize.nHeight);

                        // Need to take focus?
                        if (pX11Display->pFocusWindow == this)
                            take_focus();
                        break;
                    }

                    case UIE_HIDE:
                    {
                        bVisible        = false;
                        if (bWrapper)
                            break;

                        // Drop previously existed drawing surface
                        drop_surface();
                        break;
                    }

                    case UIE_REDRAW:
                    {
//                        lsp_trace("redraw location = %d x %d, size = %d x %d",
//                                int(ev->nLeft), int(ev->nTop),
//                                int(ev->nWidth), int(ev->nHeight));
                        break;
                    }

                    case UIE_SIZE_REQUEST:
                    {
//                        lsp_trace("size request = %d x %d", int(ev->nWidth), int(ev->nHeight));
    //                    XResizeWindow(pCore->x11display(), hWindow, ev->nWidth, ev->nHeight);
    //                    pCore->x11sync();
                        break;
                    }

                    case UIE_RESIZE:
                    {
                        if (bWrapper)
                            break;

//                        lsp_trace("new window location = %d x %d, size = %d x %d",
//                            int(ev->nLeft), int(ev->nTop),
//                            int(ev->nWidth), int(ev->nHeight));
                        sSize.nLeft         = ev->nLeft;
                        sSize.nTop          = ev->nTop;
                        sSize.nWidth        = ev->nWidth;
                        sSize.nHeight       = ev->nHeight;
                        if (pSurface != NULL)
                            pSurface->resize(sSize.nWidth, sSize.nHeight);
                        break;
                    }

                    case UIE_MOUSE_MOVE:
                    {
    //                    lsp_trace("mouse move = %d x %d", int(ev->nLeft), int(ev->nTop));
                        break;
                    }

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

                    case UIE_STATE:
                    {
                        if (ev->nCode == enState)
                            return STATUS_OK;

                        enState             = static_cast<window_state_t>(ev->nCode);
                        lsp_trace("Window state changed to: %d", int(ev->nCode));
                        break;
                    }

                    case UIE_CLOSE:
                    {
//                        lsp_trace("close request on window");
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
                {
                    handler->handle_event(ev);
                    if (gen.nType != UIE_UNKNOWN)
                        handler->handle_event(&gen);
                }

                return STATUS_OK;
            }

            status_t X11Window::set_border_style(border_style_t style)
            {
                // Update state
                enBorderStyle = style;

                switch (style)
                {
                    case BS_DIALOG:
                        sMotif.decorations  = MWM_DECOR_BORDER | MWM_DECOR_TITLE;
                        sMotif.input_mode   = MWM_INPUT_APPLICATION_MODAL;
                        sMotif.status       = 0;
                        break;

                    case BS_NONE:
                    case BS_POPUP:
                    case BS_COMBO:
                    case BS_DROPDOWN:
                        sMotif.decorations  = 0;
                        sMotif.input_mode   = MWM_INPUT_FULL_APPLICATION_MODAL;
                        sMotif.status       = 0;
                        break;

                    case BS_SINGLE:
                    case BS_SIZEABLE:
                        sMotif.decorations  = MWM_DECOR_ALL;
                        sMotif.input_mode   = MWM_INPUT_MODELESS;
                        sMotif.status       = 0;
                        break;

                    default:
                        break;
                }

                if (hWindow == None)
                    return STATUS_OK;

                // Set window state
                status_t res = set_window_state(enState);
                if (res != STATUS_OK)
                    return res;

                // Send changes to X11
                const x11_atoms_t &a = pX11Display->atoms();

                Atom atoms[32];
                int n_items = 0;

                // Set window type
                switch (style)
                {
                    case BS_DIALOG:
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_NORMAL;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_DIALOG;
                        break;

                    case BS_NONE:
                        break;

                    case BS_POPUP:
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_NORMAL;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_MENU;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_POPUP_MENU;
                        break;

                    case BS_DROPDOWN:
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_NORMAL;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_MENU;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
                        break;

                    case BS_COMBO:
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_NORMAL;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_MENU;
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_COMBO;
                        break;

                    case BS_SINGLE:
                    case BS_SIZEABLE:
                    default:
                        atoms[n_items++] = a.X11__NET_WM_WINDOW_TYPE_NORMAL;
                        break;
                }
                atoms[n_items] = 0;

//                lsp_trace("Setting _NET_WM_WINDOW_TYPE...");
                XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_WINDOW_TYPE,
                    a.X11_XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&atoms[0]),
                    n_items
                );

                // Set MOTIF hints
//                lsp_trace("Setting _MOTIF_WM_HINTS...");
                XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__MOTIF_WM_HINTS,
                    a.X11__MOTIF_WM_HINTS,
                    32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&sMotif),
                    sizeof(sMotif)/sizeof(long)
                );

                status_t result = do_update_constraints(false);
                pX11Display->flush();
                return result;
            }

            status_t X11Window::get_border_style(border_style_t *style)
            {
                if (style != NULL)
                    *style = enBorderStyle;
                return STATUS_OK;
            }

            ssize_t X11Window::left()
            {
                return sSize.nLeft;
            }

            ssize_t X11Window::top()
            {
                return sSize.nTop;
            }

            ssize_t X11Window::width()
            {
                return sSize.nWidth;
            }

            ssize_t X11Window::height()
            {
                return sSize.nHeight;
            }

            bool X11Window::is_visible()
            {
                return pSurface != NULL;
            }

            void *X11Window::handle()
            {
                return (void *)(hWindow);
            }

            size_t X11Window::screen()
            {
                return nScreen;
            }

            status_t X11Window::move(ssize_t left, ssize_t top)
            {
                if (hWindow == 0)
                    return STATUS_BAD_STATE;
                if ((sSize.nLeft == left) && (sSize.nTop == top))
                    return STATUS_OK;

                sSize.nLeft     = left;
                sSize.nTop      = top;

//                lsp_trace("left=%d, top=%d", int(left), int(top));

//                if (hParent > 0)
//                    XMoveWindow(pX11Display->x11display(), hParent, sSize.nLeft, sSize.nTop);
//                else
                status_t result = do_update_constraints(true);
                if (hParent <= 0)
                    ::XMoveWindow(pX11Display->x11display(), hWindow, sSize.nLeft, sSize.nTop);
                if (result == STATUS_OK)
                    result = do_update_constraints(false);
                if (result != STATUS_OK)
                    return result;
                pX11Display->flush();

                return STATUS_OK;
            }

            status_t X11Window::commit_size(const ws::rectangle_t *new_size)
            {
                if (hWindow == None)
                    return STATUS_OK;

//                lsp_trace("old={%d, %d}, new={%d, %d}",
//                    int(sSize.nWidth), int(sSize.nHeight),
//                    int(new_size->nWidth), int(new_size->nHeight));

                // Temporarily drop constraints
                status_t result = do_update_constraints(true);
                if (result != STATUS_OK)
                    return result;

                // Resize window if needed
//                XWindowAttributes xwa;
//                ::XGetWindowAttributes(pX11Display->x11display(), hWindow, &xwa);
//                lsp_trace("XGetWindowAttributes -> x=%d, y=%d, w=%d h=%d", int(xwa.x), int(xwa.y), int(xwa.width), int(xwa.height));

                if ((sSize.nWidth != new_size->nWidth) || (sSize.nHeight != new_size->nHeight))
                {
//                    lsp_trace("XResizeWindow(%d, %d)", int(new_size->nWidth), int(new_size->nHeight));
                    sSize.nWidth    = new_size->nWidth;
                    sSize.nHeight   = new_size->nHeight;
                    ::XResizeWindow(pX11Display->x11display(), hWindow, sSize.nWidth, sSize.nHeight);
                }

                // Enable constraints back
                result = do_update_constraints(false);
                pX11Display->flush();

                return result;
            }

            status_t X11Window::resize(ssize_t width, ssize_t height)
            {
//                lsp_trace("received: w=%d, h=%d, rw=%d, rh=%d",
//                    int(sSize.nWidth), int(sSize.nHeight),
//                    int(width), int(height));

                ws::rectangle_t new_size = sSize;

                new_size.nWidth     = width;
                new_size.nHeight    = height;

                calc_constraints(&new_size, &new_size);
//                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                return commit_size(&new_size);
            }

            status_t X11Window::set_geometry(const rectangle_t *realize)
            {
                if (hWindow == 0)
                {
//                    lsp_trace("hWindow == 0");
                    return STATUS_BAD_STATE;
                }

//                lsp_trace("received: l=%d, t=%d, w=%d, h=%d", int(realize->nLeft), int(realize->nTop), int(realize->nWidth), int(realize->nHeight));
//                lsp_trace("old: l=%d, t=%d, w=%d, h=%d", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                rectangle_t old = sSize;
                calc_constraints(&sSize, realize);

//                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                if ((old.nLeft == sSize.nLeft) &&
                    (old.nTop == sSize.nTop) &&
                    (old.nWidth == sSize.nWidth) &&
                    (old.nHeight == sSize.nHeight))
                    return STATUS_OK;

                status_t result = do_update_constraints(true);
                if (hParent > 0)
                {
                    if ((old.nWidth != sSize.nWidth) ||
                        (old.nHeight != sSize.nHeight))
                    {
//                        lsp_trace("XResizeWindow(%d, %d)", int(sSize.nWidth), int(sSize.nHeight));
                        ::XResizeWindow(pX11Display->x11display(), hWindow, sSize.nWidth, sSize.nHeight);
                    }
                }
                else
                {
                    if ((old.nLeft != sSize.nLeft) ||
                        (old.nTop != sSize.nTop) ||
                        (old.nWidth != sSize.nWidth) ||
                        (old.nHeight != sSize.nHeight))
                    {
//                        lsp_trace("XMoveResizeWindow(%d, %d, %d, %d)", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));
                        ::XMoveResizeWindow(pX11Display->x11display(), hWindow, sSize.nLeft, sSize.nTop, sSize.nWidth, sSize.nHeight);
                    }
                }

                if (result == STATUS_OK)
                    result = do_update_constraints(false);

                pX11Display->flush();

                return result;
            }

            status_t X11Window::get_geometry(rectangle_t *realize)
            {
                if (realize == NULL)
                    return STATUS_OK;

                if (hWindow != None)
                {
                    XWindowAttributes xwa;
                    ::XGetWindowAttributes(pX11Display->x11display(), hWindow, &xwa);
                    sSize.nLeft     = xwa.x;
                    sSize.nTop      = xwa.y;
                    sSize.nWidth    = xwa.width;
                    sSize.nHeight   = xwa.height;
                }

                *realize    = sSize;

                return STATUS_OK;
            }

            status_t X11Window::get_absolute_geometry(rectangle_t *realize)
            {
                if (realize == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == None)
                {
                    realize->nLeft      = 0;
                    realize->nTop       = 0;
                    realize->nWidth     = sSize.nWidth;
                    realize->nHeight    = sSize.nHeight;
                    return STATUS_BAD_STATE;
                }

                XWindowAttributes xwa;
                ::XGetWindowAttributes(pX11Display->x11display(), hWindow, &xwa);
                sSize.nLeft         = xwa.x;
                sSize.nTop          = xwa.y;
                sSize.nWidth        = xwa.width;
                sSize.nHeight       = xwa.height;

                int x, y;
                Window child;
                Display *dpy = pX11Display->x11display();
                // We do not trust XGetWindowAttributes since it can always return (0, 0) coordinates
                XTranslateCoordinates(dpy, hWindow, pX11Display->x11root(), 0, 0, &x, &y, &child);
                // lsp_trace("xy = {%d, %d}", int(x), int(y));

                realize->nLeft      = x;
                realize->nTop       = y;
                realize->nWidth     = sSize.nWidth;
                realize->nHeight    = sSize.nHeight;

                return STATUS_OK;
            }

            status_t X11Window::hide()
            {
                bVisible        = false;
                hTransientFor   = None;
                if (hWindow == 0)
                    return STATUS_BAD_STATE;

                if (pX11Display->pFocusWindow == this)
                    pX11Display->pFocusWindow = NULL;

                Display *dpy = pX11Display->x11display();
                if (nFlags & F_GRABBING)
                {
                    pX11Display->ungrab_events(this);
                    nFlags &= ~F_GRABBING;
                }
                if (nFlags & F_LOCKING)
                {
                    pX11Display->unlock_events(this);
                    nFlags &= ~F_LOCKING;
                }

                if (pSurface != NULL)
                    ::XUnmapWindow(dpy, hWindow);

                pX11Display->flush();
                return STATUS_OK;
            }

            status_t X11Window::ungrab_events()
            {
                if (hWindow == None)
                    return STATUS_BAD_STATE;
                if (!(nFlags & F_GRABBING))
                    return STATUS_NO_GRAB;

                status_t res = pX11Display->ungrab_events(this);
                nFlags &= ~F_GRABBING;
                return res;
            }

            status_t X11Window::grab_events(grab_t group)
            {
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                if (nFlags & F_GRABBING)
                    return STATUS_OK;

                status_t res = pX11Display->grab_events(this, group);
                if (res == STATUS_OK)
                    nFlags |= F_GRABBING;

                return res;
            }

            bool X11Window::is_grabbing_events() const
            {
                return nFlags & F_GRABBING;
            }

            status_t X11Window::show(IWindow *over)
            {
                if (hWindow == 0)
                    return STATUS_BAD_STATE;
                if (pSurface != NULL)
                    return STATUS_OK;

                ::Window transient_for = None;
                Display *dpy = pX11Display->x11display();
                X11Window *wnd = NULL;
                if (over != NULL)
                {
                    wnd = static_cast<X11Window *>(over);
                    if (wnd->hParent != None)
                        transient_for = wnd->hParent;
                    else if (wnd->hWindow > 0)
                        transient_for = wnd->hWindow;
                }
                hTransientFor   = transient_for;

//                lsp_trace("Showing window %lx as transient for %lx", hWindow, transient_for);
                ::XSetTransientForHint(dpy, hWindow, transient_for);
                ::XMapRaised(dpy, hWindow);
                if (hTransientFor != None)
                {
                    XWindowChanges cw;
                    cw.x            = 0;
                    cw.y            = 0;
                    cw.width        = 0;
                    cw.height       = 0;
                    cw.border_width = 0;
                    cw.sibling      = hTransientFor;
                    cw.stack_mode   = Above;
                    ::XConfigureWindow(dpy, hWindow, CWStackMode, &cw);
                }

                pX11Display->sync();
//                XWindowAttributes atts;
//                XGetWindowAttributes(pX11Display->x11display(), hWindow, &atts);
//                lsp_trace("window x=%d, y=%d", atts.x, atts.y);

                set_border_style(enBorderStyle);
                set_window_actions(nActions);

                switch (enBorderStyle)
                {
                    case BS_POPUP:
                    case BS_COMBO:
                    case BS_DROPDOWN:
//                        pX11Display->grab_events(this);
//                        nFlags |= F_GRABBING;
                        break;
                    case BS_DIALOG:
                        if (wnd != NULL)
                        {
                            pX11Display->lock_events(this, wnd);
                            nFlags |= F_LOCKING;
                        }
                        break;
                    default:
                        break;
                }

                // Bring window to top
                if (enState != WS_MINIMIZED)
                {
                    XEvent ev;
                    ev.xclient.type = ClientMessage;
                    ev.xclient.serial = 0;
                    ev.xclient.send_event = True;
                    ev.xclient.message_type = pX11Display->atoms().X11__NET_ACTIVE_WINDOW;
                    ev.xclient.window = hWindow;
                    ev.xclient.format = 32;

                    XSendEvent(
                        dpy,
                        pX11Display->x11root(),
                        False,
                        SubstructureRedirectMask | SubstructureNotifyMask,
                        &ev);
                }

                return STATUS_OK;
            }

            status_t X11Window::show()
            {
                return show(NULL);
            }

            status_t X11Window::set_left(ssize_t left)
            {
                return move(left, sSize.nTop);
            }

            status_t X11Window::set_top(ssize_t top)
            {
                return move(sSize.nLeft, top);
            }

            ssize_t X11Window::set_width(ssize_t width)
            {
                return resize(width, sSize.nHeight);
            }

            ssize_t X11Window::set_height(ssize_t height)
            {
                return resize(sSize.nWidth, height);
            }

            status_t X11Window::set_size_constraints(const size_limit_t *c)
            {
                sConstraints    = *c;
                if (sConstraints.nMinWidth == 0)
                    sConstraints.nMinWidth  = 1;
                if (sConstraints.nMinHeight == 0)
                    sConstraints.nMinHeight = 1;

                ws::rectangle_t new_size;
                calc_constraints(&new_size, &sSize);
//                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                return commit_size(&new_size);
            }

            status_t X11Window::get_size_constraints(size_limit_t *c)
            {
                *c = sConstraints;
                return STATUS_OK;
            }

            void X11Window::send_focus_event()
            {
                Display *dpy = pX11Display->x11display();

                XEvent  ev;
                ev.xclient.type         = ClientMessage;
                ev.xclient.serial       = 0;
                ev.xclient.send_event   = True;
                ev.xclient.display      = dpy;
                ev.xclient.window       = pX11Display->x11root();
                ev.xclient.message_type = pX11Display->atoms().X11__NET_ACTIVE_WINDOW;
                ev.xclient.format       = 32;
                ev.xclient.data.l[0]    = ((enBorderStyle == BS_POPUP) || (enBorderStyle == BS_COMBO) || (enBorderStyle == BS_DROPDOWN)) ? 2 : 1;
                ev.xclient.data.l[1]    = CurrentTime;
                ev.xclient.data.l[2]    = hWindow;
                ev.xclient.data.l[3]    = 0;
                ev.xclient.data.l[4]    = 0;

                ::XSendEvent(dpy, pX11Display->x11root(), True, NoEventMask, &ev);
            }

            status_t X11Window::take_focus()
            {
                if ((hWindow == 0) || (!bVisible))
                {
                    pX11Display->pFocusWindow   = this;
                    return STATUS_OK;
                }

//                lsp_trace("focusing window: focus=%s", (focus) ? "true" : "false");
                if (pX11Display->pFocusWindow == this)
                    pX11Display->pFocusWindow   = NULL;

                lsp_trace("Calling set_input_focus this=%p, handle=%lx...", this, long(hWindow));
                bool res = pX11Display->set_input_focus(hWindow);

                lsp_trace("Sending focus event...");
                send_focus_event();

                lsp_trace("result = %s", (res) ? "success" : "unknown_error");

                return (res) ? STATUS_OK : STATUS_UNKNOWN_ERR;
            }

            status_t X11Window::set_caption(const char *caption)
            {
                if (caption == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                // Set legacy property
                const x11_atoms_t &a = pX11Display->atoms();
                LSPString text;
                if (text.set_utf8(caption))
                {
                    const char *ascii = text.get_ascii();
                    ::XChangeProperty(
                        pX11Display->x11display(),
                        hWindow,
                        a.X11_XA_WM_NAME,
                        a.X11_XA_STRING,
                        8,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char *>(ascii),
                        ::strlen(ascii)
                    );
                }

                // Set modern property for window manager
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_NAME,
                    a.X11_UTF8_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(caption),
                    ::strlen(caption)
                );
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_ICON_NAME,
                    a.X11_UTF8_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(caption),
                    ::strlen(caption)
                );

                pX11Display->flush();

                return STATUS_OK;
            }

            status_t X11Window::set_caption(const LSPString *caption)
            {
                if (caption == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                const x11_atoms_t &a = pX11Display->atoms();

                // Set legacy property
                const char *ascii = caption->get_ascii();
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11_XA_WM_NAME,
                    a.X11_XA_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(ascii),
                    ::strlen(ascii)
                );

                // Set modern property for window manager
                const char *utf8 = caption->get_utf8();
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_NAME,
                    a.X11_UTF8_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(utf8),
                    ::strlen(utf8)
                );
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_ICON_NAME,
                    a.X11_UTF8_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(utf8),
                    ::strlen(utf8)
                );

                pX11Display->flush();

                return STATUS_OK;
            }

            status_t X11Window::get_caption(char *text, size_t len)
            {
                if (text == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (len < 1)
                    return STATUS_TOO_BIG;
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                unsigned long count = 0, left = 0;
                Atom ret;
                int fmt;
                unsigned char* data;

                const x11_atoms_t &a = pX11Display->atoms();

                int result = XGetWindowProperty(
                    pX11Display->x11display() /* display */,
                    hWindow /* window */,
                    a.X11__NET_WM_NAME /* property */,
                    0           /* long_offset */,
                    ~0L         /* long_length */,
                    False       /* delete */,
                    a.X11_UTF8_STRING  /* req_type */,
                    &ret        /* actual_type_return */,
                    &fmt        /* actual_format_return */,
                    &count      /* nitems_return */,
                    &left       /* bytes_after_return */,
                    &data       /* prop_return */
                );

                if (result != Success)
                    return STATUS_UNKNOWN_ERR;

                if ((ret != a.X11_UTF8_STRING) || (count <= 0) || (data == NULL))
                {
                    XFree(data);
                    text[0] = '\0';
                    return STATUS_OK;
                }
                else if (count >= len)
                {
                    XFree(data);
                    return STATUS_TOO_BIG;
                }

                memcpy(text, data, count);
                text[count] = '\0';
                return STATUS_OK;
            }

            status_t X11Window::get_caption(LSPString *caption)
            {
                if (caption == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                unsigned long count = 0, left = 0;
                Atom ret;
                int fmt;
                unsigned char* data;

                const x11_atoms_t &a = pX11Display->atoms();

                int result = XGetWindowProperty(
                    pX11Display->x11display() /* display */,
                    hWindow /* window */,
                    a.X11__NET_WM_NAME /* property */,
                    0           /* long_offset */,
                    ~0L         /* long_length */,
                    False       /* delete */,
                    a.X11_UTF8_STRING  /* req_type */,
                    &ret        /* actual_type_return */,
                    &fmt        /* actual_format_return */,
                    &count      /* nitems_return */,
                    &left       /* bytes_after_return */,
                    &data       /* prop_return */
                );

                if (result != Success)
                    return STATUS_UNKNOWN_ERR;
                lsp_finally{
                    if (data != NULL)
                        XFree(data);
                };

                if ((ret != a.X11_UTF8_STRING) || (count <= 0) || (data == NULL))
                {
                    caption->clear();
                    return STATUS_OK;
                }

                return (caption->set_utf8(reinterpret_cast<char *>(data), count)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t X11Window::set_icon(const void *bgra, size_t width, size_t height)
            {
                if (hWindow <= 0)
                    return STATUS_BAD_STATE;

                const size_t n = width * height;
                unsigned long *buffer = reinterpret_cast<unsigned long *>(malloc(sizeof(unsigned long) * (n + 2)));
                if (buffer == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { free(buffer); };

                buffer[0] = width;
                buffer[1] = height;

                const uint32_t *ptr = reinterpret_cast<const uint32_t *>(bgra);
                unsigned long *dst  = &buffer[2];

                for (size_t i=0; i<n; ++i)
                    *(dst++) = LE_TO_CPU(*(ptr++));

                const x11_atoms_t &a = pX11Display->atoms();

                XChangeProperty(
                    pX11Display->x11display(), hWindow,
                    a.X11__NET_WM_ICON, a.X11_XA_CARDINAL, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(buffer), n + 2);

                return STATUS_OK;
            }

            status_t X11Window::get_window_actions(size_t *actions)
            {
                if (actions == NULL)
                    return STATUS_BAD_ARGUMENTS;
                *actions = nActions;
                return STATUS_OK;
            }

            status_t X11Window::set_window_actions(size_t actions)
            {
                nActions            = actions;

                // Update legacy _MOTIF_WM_HINTS
                sMotif.functions    = 0;

                // Functions
                if (actions & WA_MOVE)
                    sMotif.functions       |= MWM_FUNC_MOVE;
                if (actions & WA_RESIZE)
                    sMotif.functions       |= MWM_FUNC_RESIZE;
                if (actions & WA_MINIMIZE)
                    sMotif.functions       |= MWM_FUNC_MINIMIZE;
                if (actions & WA_MAXIMIZE)
                    sMotif.functions       |= MWM_FUNC_MAXIMIZE;
                if (actions & WA_CLOSE)
                    sMotif.functions       |= MWM_FUNC_CLOSE;

                if (hWindow == None)
                    return STATUS_OK;

                // Set window actions
                Atom atoms[10];
                size_t n_items = 0;
                const x11_atoms_t &a = pX11Display->atoms();

                // Update _NET_WM_ALLOWED_ACTIONS
                #define TR_ACTION(from, to)    \
                    if (actions & from) \
                        atoms[n_items++] = a.X11__NET_WM_ACTION_ ## to;

                TR_ACTION(WA_MOVE, MOVE);
                TR_ACTION(WA_RESIZE, RESIZE);
                TR_ACTION(WA_MINIMIZE, MINIMIZE);
                TR_ACTION(WA_MAXIMIZE, MAXIMIZE_HORZ);
                TR_ACTION(WA_MAXIMIZE, MAXIMIZE_VERT);
                TR_ACTION(WA_CLOSE, CLOSE);
                TR_ACTION(WA_STICK, STICK);
                TR_ACTION(WA_SHADE, SHADE);
                TR_ACTION(WA_FULLSCREEN, FULLSCREEN);
                TR_ACTION(WA_CHANGE_DESK, CHANGE_DESKTOP);

                #undef TR_ACTION

//                lsp_trace("Setting _NET_WM_ALLOWED_ACTIONS...");
                XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_ALLOWED_ACTIONS,
                    a.X11_XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&atoms[0]),
                    n_items
                );


                // Send _MOTIF_WM_HINTS
//                lsp_trace("Setting _MOTIF_WM_HINTS...");
                XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__MOTIF_WM_HINTS,
                    a.X11__MOTIF_WM_HINTS,
                    32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&sMotif),
                    sizeof(sMotif)/sizeof(long)
                );

                pX11Display->flush();

                return STATUS_OK;
            }

            status_t X11Window::set_mouse_pointer(mouse_pointer_t pointer)
            {
                if (hWindow <= 0)
                    return STATUS_BAD_STATE;

                Cursor cur = pX11Display->get_cursor(pointer);
                if (cur == None)
                    return STATUS_UNKNOWN_ERR;
                XDefineCursor(pX11Display->x11display(), hWindow, cur);
                XFlush(pX11Display->x11display());
                enPointer = pointer;
                return STATUS_OK;
            }

            mouse_pointer_t X11Window::get_mouse_pointer()
            {
                return enPointer;
            }

            status_t X11Window::set_class(const char *instance, const char *wclass)
            {
                if ((instance == NULL) || (wclass == NULL))
                    return STATUS_BAD_ARGUMENTS;

                size_t l1 = ::strlen(instance);
                size_t l2 = ::strlen(wclass);

                char *dup = reinterpret_cast<char *>(::malloc((l1 + l2 + 2) * sizeof(char)));
                if (dup == NULL)
                    return STATUS_NO_MEM;

                ::memcpy(dup, instance, l1+1);
                ::memcpy(&dup[l1+1], wclass, l2+1);

                const x11_atoms_t &a = pX11Display->atoms();
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11_XA_WM_CLASS,
                    a.X11_XA_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(dup),
                    (l1 + l2 + 2)
                );

                ::free(dup);
                return STATUS_OK;
            }

            status_t X11Window::set_role(const char *wrole)
            {
                if (wrole == NULL)
                    return STATUS_BAD_ARGUMENTS;

                const x11_atoms_t &a = pX11Display->atoms();
                ::XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11_WM_WINDOW_ROLE,
                    a.X11_XA_STRING,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char *>(wrole),
                    ::strlen(wrole)
                );

                return STATUS_OK;
            }

            void *X11Window::parent() const
            {
                if (hWindow == None)
                    return NULL;

                Window root = None, parent = None, *children = NULL;
                lsp_finally {
                    if (children != NULL)
                        XFree(children);
                };
                unsigned int num_children;

                XQueryTree(
                    pX11Display->x11display(),
                    hWindow, &root, &parent,
                    &children, &num_children);

                return (parent != root) ? reinterpret_cast<void *>(parent) : NULL;
            }

            status_t X11Window::set_parent(void *parent)
            {
                if (hWindow == None)
                    return STATUS_BAD_STATE;

                Window parent_wnd = (parent != NULL) ?
                    reinterpret_cast<Window>(parent) :
                    pX11Display->x11root();

                XReparentWindow(
                    pX11Display->x11display(),
                    hWindow,
                    parent_wnd,
                    sSize.nLeft,
                    sSize.nTop);

                return STATUS_OK;
            }

            status_t X11Window::get_window_state(window_state_t *state)
            {
                *state = enState;
                return STATUS_OK;
            }

            template <class T>
            inline bool find_atom(Atom lookup, const T *list, size_t count)
            {
                for (size_t i=0; i<count; ++i)
                    if (list[i] == lookup)
                        return true;
                return false;
            }

            status_t X11Window::set_window_state(window_state_t state)
            {
                // Update state
                enState     = state;
                if (hWindow == None)
                    return STATUS_OK;

                lsp_trace("hWindow = 0x%lx, state = %d", hWindow, state);

                // Set window state
                Atom atoms[32];
                size_t n_items = 0;
                const x11_atoms_t &a = pX11Display->atoms();

                switch (enBorderStyle)
                {
                    case BS_DIALOG:         // Not resizable; no minimize/maximize menu
                        atoms[n_items++] = a.X11__NET_WM_STATE_MODAL;
                        if (hTransientFor != None)
                            atoms[n_items++] = a.X11__NET_WM_STATE_SKIP_TASKBAR;
                        break;
                    case BS_NONE:           // Not resizable; no visible border line
                    case BS_POPUP:
                    case BS_COMBO:
                    case BS_DROPDOWN:
                        atoms[n_items++] = a.X11__NET_WM_STATE_ABOVE;
                        atoms[n_items++] = a.X11__NET_WM_STATE_SKIP_TASKBAR;
                        break;

                    case BS_SINGLE:         // Not resizable; minimize/maximize menu
                    case BS_SIZEABLE:       // Standard resizable border
                        break;
                }

                if (enState == WS_MAXIMIZED)
                {
                    atoms[n_items++]    = a.X11__NET_WM_STATE_MAXIMIZED_HORZ;
                    atoms[n_items++]    = a.X11__NET_WM_STATE_MAXIMIZED_VERT;
                }
                else if (enState == WS_MINIMIZED)
                    atoms[n_items++]    = a.X11__NET_WM_STATE_HIDDEN;

                // Read old window state before changing it
                uint8_t *data   = NULL;
                Atom xtype      = 0;
                size_t size     = 0;

                lsp_trace("hWindow = 0x%lx, state = %d", hWindow, state);
                pX11Display->read_property(hWindow, a.X11__NET_WM_STATE, a.X11_XA_ATOM, &data, &size, &xtype);
                lsp_finally {
                    if (data != NULL)
                        free(data);
                };

                // Set new window state
//                lsp_trace("Setting _NET_WM_STATE...");
                XChangeProperty(
                    pX11Display->x11display(),
                    hWindow,
                    a.X11__NET_WM_STATE,
                    a.X11_XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&atoms[0]),
                    n_items
                );

                Display *dpy        = pX11Display->x11display();
                Window root         = pX11Display->hRootWnd;

                XEvent xe;
                XClientMessageEvent *xev = &xe.xclient;
                xev->type           = ClientMessage;
                xev->serial         = 0;
                xev->send_event     = True;
                xev->display        = dpy;
                xev->window         = hWindow;
                xev->message_type   = a.X11__NET_WM_STATE;
                xev->format         = 32;
                xev->data.l[0]      = 0;        // _NET_WM_STATE_REMOVE
                xev->data.l[1]      = 0;
                xev->data.l[2]      = 0;
                xev->data.l[3]      = 1;        // Source indicator: Normal application
                xev->data.l[4]      = 0;

                const uint32_t *old_atoms   = reinterpret_cast<const uint32_t *>(data); // read_property compresses long -> uint32_t
                const size_t old_n_items    = size / sizeof(uint32_t);

                // Remove old properties
                bool full_removed   = false;
                for (size_t i=0; i<old_n_items; ++i)
                {
                    const Atom atom     = old_atoms[i];
                    if (!find_atom(atom, atoms, n_items))
                    {
                        if ((atom == a.X11__NET_WM_STATE_MAXIMIZED_HORZ) ||
                            (atom == a.X11__NET_WM_STATE_MAXIMIZED_VERT))
                        {
                            if (!full_removed)
                            {
                                xev->data.l[1]  = a.X11__NET_WM_STATE_MAXIMIZED_HORZ;
                                xev->data.l[2]  = a.X11__NET_WM_STATE_MAXIMIZED_VERT;
                                full_removed    = true;

                                ::XSendEvent(dpy, root, False, SubstructureNotifyMask, &xe);
                            }
                        }
                        else
                        {
                            xev->data.l[1]  = atom;
                            xev->data.l[2]  = 0;

                            ::XSendEvent(dpy, root, False, SubstructureNotifyMask, &xe);
                        }
                    }
                }

                // Add new properties
                xev->data.l[0]      = 1;        // _NET_WM_STATE_ADD
                bool full_added     = false;

                for (size_t i=0; i<n_items; ++i)
                {
                    const Atom atom     = atoms[i];
                    if (!find_atom(atom, old_atoms, old_n_items))
                    {
                        if ((atom == a.X11__NET_WM_STATE_MAXIMIZED_HORZ) ||
                            (atom == a.X11__NET_WM_STATE_MAXIMIZED_VERT))
                        {
                            if (!full_added)
                            {
                                xev->data.l[1]  = a.X11__NET_WM_STATE_MAXIMIZED_HORZ;
                                xev->data.l[2]  = a.X11__NET_WM_STATE_MAXIMIZED_VERT;
                                full_added      = true;

                                ::XSendEvent(dpy, root, False, SubstructureNotifyMask, &xe);
                            }
                        }
                        else
                        {
                            xev->data.l[1]  = atom;
                            xev->data.l[2]  = 0;

                            ::XSendEvent(dpy, root, False, SubstructureNotifyMask, &xe);
                        }
                    }
                }

                // Iconify window if state is minimized
                if (enState == WS_MINIMIZED)
                {
                    xev->message_type   = a.X11_WM_CHANGE_STATE;
                    xev->format         = 32;
                    xev->data.l[0]      = IconicState;
                    xev->data.l[1]      = 0;
                    xev->data.l[2]      = 0;
                    xev->data.l[3]      = 0;
                    xev->data.l[4]      = 0;

                    ::XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xe);
                }

                pX11Display->flush();

                return STATUS_OK;
            }

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

/* Example of changing window's icon
 https://stackoverflow.com/questions/10699927/xlib-argb-window-icon

    // gcc -Wall -Wextra -Og -lX11 -lstdc++ -L/usr/X11/lib -o native foo.c
    #include <stdlib.h>
    #include <X11/Xlib.h>
    int main( int argc, char **argv )
    {
        unsigned long buffer[] = {
                16, 16,
                4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 338034905, 3657433343, 0, 184483840, 234881279, 3053453567, 3221225727, 1879048447, 0, 0, 0, 0, 0, 0, 0, 1224737023, 3305111807, 3875537151,0, 0, 2063597823, 1291845887, 0, 67109119, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 50266112, 3422552319, 0, 0, 3070230783, 2063597823, 2986344703, 771752191, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3422552319, 0, 0, 3372220671, 1509949695, 704643327, 3355443455, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 3422552319, 0, 134152192, 3187671295, 251658495, 0, 3439329535, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3422552319, 0, 0, 2332033279, 1342177535, 167772415, 3338666239, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 3422552319, 0, 0, 436207871, 3322085628, 3456106751, 1375731967, 4278255360, 4026597120, 3758161664, 3489726208, 3204513536, 2952855296, 2684419840, 2399207168, 2130771712, 1845559040, 1593900800, 1308688128, 1040252672, 755040000, 486604544, 234946304, 4278255360, 4043374336, 3774938880, 3506503424, 3221290752, 2952855296, 2667642624, 2399207168, 2130771712, 1862336256, 1627453957, 1359017481, 1073805064, 788591627, 503379721, 218169088, 4278255360, 4043374336, 3758161664, 3506503424, 3221290752, 2952855296, 2684419840, 2415984384, 2130771712, 1862336256, 1577123584, 1308688128, 1040252672, 755040000, 486604544, 218169088, 4278190335, 4026532095, 3758096639, 3489661183, 3221225727, 2952790271, 2667577599, 2415919359, 2130706687, 1862271231, 1593835775, 1325400319, 1056964863, 771752191, 520093951, 234881279, 4278190335, 4026532095, 3758096639, 3489661183, 3221225727, 2952790271, 2667577599, 2415919359, 2130706687, 1862271231, 1593835775, 1325400319, 1056964863, 771752191, 503316735, 234881279, 4278190335, 4026532095, 3758096639, 3489661183, 3221225727, 2952790271, 2684354815, 2399142143, 2130706687, 1862271231, 1593835775, 1325400319, 1040187647, 771752191, 520093951, 234881279, 4294901760, 4043243520, 3774808064, 3506372608, 3221159936, 2952724480, 2684289024, 2399076352, 2147418112, 1862205440, 1593769984, 1308557312, 1040121856, 771686400, 503250944, 234815488, 4294901760, 4060020736, 3758030848, 3506372608, 3221159936, 2952724480, 2684289024, 2415853568, 2130640896, 1862205440, 1593769984, 1308557312, 1040121856, 771686400, 503250944, 234815488, 4294901760, 4043243520, 3774808064, 3489595392, 3237937152, 2952724480, 2684289024, 2415853568, 2147418112, 1862205440, 1593769984, 1325334528, 1056899072, 788463616, 503250944, 234815488,
                32, 32,
                4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 0, 0, 0, 0, 0, 0, 0, 0, 268369920, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 1509949695, 3120562431, 4009754879, 4194304255, 3690987775, 2130706687, 83886335, 0, 50331903, 1694499071, 3170894079, 3992977663, 4211081471, 3657433343, 1879048447, 16777471, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3087007999, 2281701631, 1191182591, 1040187647, 2030043391, 4127195391, 2566914303, 0, 16777471, 3254780159, 2181038335, 1191182591, 973078783, 2030043391,4177527039, 2130706687, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 0, 0, 0, 0, 2214592767, 4093640959, 0, 0, 0, 0, 0, 0, 0, 2298478847, 3909091583, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2214592767, 3607101695, 0, 0, 0, 0, 0, 0, 0, 1946157311, 4093640959, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 0, 536871167, 1191182591, 2281701631,3019899135, 637534463, 0, 0, 0, 100597760, 251592704, 33488896, 0, 3321889023, 2919235839, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2550137087, 4278190335, 4278190335, 3405775103, 570425599, 0, 0, 0, 0, 0, 0, 2046820607, 4043309311, 620757247, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 33488896, 0, 0, 218104063, 1291845887, 3841982719, 3388997887, 0, 0, 0, 0, 0, 1996488959, 4093640959, 1073742079, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1761607935, 4278190335, 150995199, 0, 0, 67109119, 2550137087, 3909091583, 889192703, 0, 0, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 0, 0, 0, 0, 2181038335, 3925868799, 0, 0, 218104063, 3070230783, 3623878911, 570425599, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 805306623, 3288334591, 1795162367, 1040187647, 1023410431, 2231369983, 4211081471, 1694499071, 0, 369099007, 3456106751, 3825205503, 1174405375, 872415487, 872415487, 872415487, 872415487, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4293984270, 2046951677, 3422552319, 4110418175, 4177527039, 3405775103, 1409286399, 0, 0, 1409286399, 4278190335, 4278190335, 4278190335, 4278190335, 4278190335, 4278190335, 4278190335, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760,4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 4294901760, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4278255360, 4144037632, 4009819904, 3875602176, 3741384448, 3607166720, 3472948992, 3338731264, 3204513536, 3053518592, 2936078080, 2801860352, 2650865408, 2516647680, 2382429952, 2264989440, 2113994496, 1996553984, 1862336256, 1728118528, 1577123584, 1459683072, 1325465344, 1191247616, 1040252672, 922812160, 771817216, 637599488, 503381760, 385941248, 234946304, 100728576, 4278255360, 4144037632, 4009819904, 3875602176, 3724607232, 3607166720, 3472948992, 3338731264, 3204513536, 3070295808, 2936078080, 2801860352, 2667642624, 2516647680, 2399207168, 2264989440, 2130771712, 1996553984, 1845559040, 1728118528, 1593900800, 1459683072, 1308688128, 1191247616, 1057029888, 922812160, 788594432, 637599488, 503381760, 369164032, 234946304, 117505792, 4278255360, 4144037632, 4009819904, 3875602176, 3741384448, 3607166720, 3472948992, 3338731264, 3204513536, 3053518592, 2919300864, 2801860352, 2650865408, 2533424896, 2399207168, 2264989440, 2113994496, 1996553984, 1862336256, 1728118528,1593900800, 1459683072, 1325465344, 1191247616, 1040252672, 906034944, 771817216, 654376704, 503381760, 369164032, 234946304, 117505792, 4278255360, 4144037632, 4009819904, 3858824960, 3741384448, 3607166720, 3472948992, 3338731264, 3204513536, 3070295808, 2936078080, 2801860352, 2667642624, 2533424896, 2382429952, 2264989440, 2130771712, 1979776768, 1862336256, 1728118528, 1577123584, 1442905856, 1325465344, 1191247616, 1040252672, 922812160, 771817216, 637599488, 503381760, 369164032, 234946304, 100728576, 4278255360, 4144037632, 4009819904, 3875602176, 3741384448, 3607166720, 3472948992, 3338731264, 3204513536, 3070295808, 2919300864, 2801860352, 2667642624, 2533424896, 2399207168, 2264989440, 2113994496, 1996553984, 1862336256, 1728118528, 1593900800, 1442905856, 1342241795, 1174470400, 1057029888, 906034944, 788594432, 654376704, 503381760, 385941248, 251723520, 100728576, 4278190335, 4160749823, 4026532095, 3892314367, 3741319423, 3623878911, 3472883967, 3338666239, 3221225727, 3070230783, 2952790271, 2818572543, 2667577599, 2533359871, 2399142143, 2264924415, 2147483903, 1996488959, 1862271231, 1728053503, 1593835775, 1459618047, 1325400319, 1191182591, 1056964863, 922747135, 788529407, 654311679, 520093951,385876223, 251658495, 117440767, 4278190335, 4160749823, 4026532095, 3892314367, 3741319423, 3623878911, 3489661183, 3355443455, 3221225727, 3087007999, 2936013055, 2801795327, 2667577599, 2533359871, 2399142143, 2281701631, 2130706687, 1996488959, 1862271231, 1728053503, 1593835775,1459618047, 1325400319, 1191182591, 1056964863, 922747135, 788529407, 654311679, 520093951, 385876223, 234881279, 100663551, 4278190335, 4160749823, 4026532095, 3892314367, 3758096639, 3623878911, 3489661183, 3355443455, 3221225727, 3087007999, 2936013055, 2801795327, 2667577599, 2550137087, 2415919359, 2264924415, 2130706687, 1996488959, 1862271231, 1728053503, 1593835775, 1459618047, 1325400319, 1191182591, 1056964863, 922747135, 788529407, 654311679, 503316735, 369099007, 251658495, 100663551, 4278190335, 4160749823, 4026532095, 3892314367, 3758096639, 3623878911, 3489661183, 3355443455, 3204448511, 3087007999, 2936013055, 2818572543, 2667577599, 2533359871, 2399142143, 2264924415, 2130706687, 1996488959, 1879048447, 1728053503, 1593835775, 1459618047, 1325400319, 1191182591, 1056964863, 922747135, 788529407, 654311679, 520093951, 385876223, 251658495, 117440767, 4278190335, 4160749823, 4026532095, 3892314367, 3758096639, 3623878911, 3489661183, 3355443455, 3221225727, 3087007999, 2952790271, 2818572543, 2667577599, 2533359871, 2399142143, 2264924415, 2147483903, 2013266175, 1862271231, 1744830719, 1610612991, 1476395263, 1342177535, 1191182591, 1056964863, 922747135, 788529407, 654311679, 520093951, 385876223, 251658495, 100663551, 4294901760, 4160684032, 4026466304, 3909025792, 3774808064, 3623813120, 3489595392, 3355377664, 3237937152, 3103719424, 2952724480, 2818506752, 2684289024, 2550071296, 2415853568, 2281635840, 2147418112, 2013200384, 1878982656, 1744764928, 1593769984, 1476329472,1325334528, 1207894016, 1056899072, 939458560, 788463616, 654245888, 520028160, 385810432, 251592704, 117374976, 4294901760, 4177461248, 4043243520, 3909025792, 3774808064, 3640590336, 3506372608, 3355377664, 3221159936, 3086942208, 2952724480, 2818506752, 2701066240, 2550071296, 2415853568, 2281635840, 2147418112, 2013200384, 1878982656, 1727987712, 1610547200, 1476329472, 1325334528, 1191116800, 1073676288, 922681344, 788463616, 654245888, 520028160, 385810432, 251592704, 100597760, 4294901760, 4177461248, 4043243520, 3909025792, 3774808064, 3640590336, 3489595392, 3372154880, 3237937152, 3103719424, 2952724480, 2818506752, 2700935170, 2550071296, 2415853568, 2281635840, 2147418112, 2013200384, 1878982656, 1744764928, 1610547200, 1459552256, 1342111744, 1191116800, 1056899072, 922681344, 788463616, 671023104, 520028160, 385810432, 251592704, 100597760, 4294901760, 4177461248, 4043243520, 3909025792, 3774808064, 3640590336, 3489595392, 3372154880, 3237937152, 3086942208, 2969501696, 2818506752, 2684289024, 2550071296, 2432630784, 2281635840, 2147418112, 2013200384, 1862205440, 1744764928, 1610547200, 1476329472, 1342111744, 1191116800, 1056899072, 922681344, 788463616, 654245888, 520028160, 385810432, 251592704, 117374976, 4294901760, 4177461248, 4043243520, 3909025792, 3774808064, 3623813120, 3506372608, 3372154880, 3237937152, 3103719424, 2952724480, 2835283968, 2684289024, 2550071296, 2432630784, 2281635840, 2147418112, 2046492676, 1862205440, 1744764928, 1610547200, 1476329472, 1342111744,1207894016, 1056899072, 939458560, 788463616, 654245888, 536281096, 385810432, 251592704, 134152192,
        };
        Display *d = XOpenDisplay(0);
        int s = DefaultScreen(d);
        Atom net_wm_icon = XInternAtom(d, "_NET_WM_ICON", False);
        Atom cardinal = XInternAtom(d, "CARDINAL", False);
        Window w;
        XEvent e; w = XCreateWindow(d, RootWindow(d, s), 0, 0, 200, 200, 0,
        CopyFromParent, InputOutput, CopyFromParent, 0, 0);
        int length = 2 + 16 * 16 + 2 + 32 * 32;
        XChangeProperty(d, w, net_wm_icon, cardinal, 32, PropModeReplace, (const unsigned char*) buffer, length);
        XMapWindow(d, w);
        while(1)
        XNextEvent(d, &e);
        (void)argc, (void)argv;
    }
 */

#endif /* USE_LIBX11 */
