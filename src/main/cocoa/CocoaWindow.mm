/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#import <Cocoa/Cocoa.h>
#include <iostream>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/new.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/runtime/system.h>

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/cocoa/CocoaDisplay.h>
#include <private/cocoa/CocoaWindow.h>
#include <private/cocoa/CocoaCairoSurface.h>
#include <private/cocoa/CocoaCairoView.h>

#include <cairo/cairo.h>
#include <cairo/cairo-quartz.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            
            CocoaWindow::CocoaWindow(CocoaDisplay *dpy, IEventHandler *handler, bool wrapper): ws::IWindow(dpy, handler)
            {
                pCocoaDisplay = dpy;
               
                nActions                = WA_SINGLE;
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
            }

            CocoaWindow::~CocoaWindow()
            {
                pCocoaDisplay   = NULL;
                pCocoaWindow    = NULL;
            }

            status_t CocoaWindow::init()
            {
                // Initialize parent class
                status_t res = IWindow::init();
                if (res != STATUS_OK)
                    return res;

                // Create a window
                NSRect frame = NSMakeRect(100, 100, 400, 300);
                NSWindow *window = [[NSWindow alloc]
                                    initWithContentRect:frame
                                            styleMask:(NSWindowStyleMaskTitled |
                                                        NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                defer:NO];
                
                pCocoaWindow = window;
                [pCocoaWindow makeKeyAndOrderFront:nil];
                
                // Create a cocoa view and set it to window
                CocoaCairoView *view = [[CocoaCairoView alloc] initWithFrame:frame];
                pCocoaView = view;
                [pCocoaWindow setContentView:pCocoaView];

                return STATUS_OK;
            }

            void CocoaWindow::destroy()
            {
                
                if (pCocoaDisplay != NULL)
                    pCocoaDisplay->vWindows.qpremove(this);

                // Surface related
                if (pSurface != NULL)
                {
                    pSurface->destroy();
                    delete pSurface;
                    pSurface = NULL;
                }
                /*
                if (!bWrapper)
                    DestroyWindow(hWindow);
                */
                //pCocoaDisplay = NULL;

                IWindow::destroy();
            }

            NSWindow *CocoaWindow::nswindow() const {
                return pCocoaWindow;
            }

            //TODO
            ISurface *CocoaWindow::create_surface(CocoaDisplay *display, size_t width, size_t height)
            {
                ISurface *result = NULL;

                // Trigger a redraw in NSView - just for testing
                [(CocoaCairoView *)pCocoaView triggerRedraw];
             
/*
                if (result == NULL)
                {
                    CGContextRef cg = [[NSGraphicsContext currentContext] CGContext];
                    result = new CocoaCairoSurface(display, cg, width, height);
                    if (result != NULL)
                        lsp_trace("Using CocoaCairoSurface ptr=%p", result);
                }
*/
                return result;
            }


            ISurface *CocoaWindow::get_surface()
            {
                if (bWrapper)
                    return NULL;
                return pSurface;
            }

            void CocoaWindow::drop_surface()
            {
                if (pSurface != NULL)
                {
                    pSurface->destroy();
                    delete pSurface;
                    pSurface = NULL;
                }
            }

            //TODO: Translate all Cursors!
            static NSCursor* translate_cursor(mouse_pointer_t pointer) {
                using namespace lsp::ws;
                switch (pointer) {
                    case MP_ARROW:      return [NSCursor arrowCursor];
                    case MP_IBEAM:      return [NSCursor IBeamCursor];
                    case MP_CROSS:      return [NSCursor crosshairCursor];
                    case MP_HAND:       return [NSCursor openHandCursor];
                    default:            return [NSCursor arrowCursor];;
                }
            }

            status_t CocoaWindow::set_mouse_pointer(mouse_pointer_t pointer)
            {
                if (pCocoaWindow == nullptr)  // NSWindow*
                    return STATUS_BAD_STATE;
                if (enPointer == pointer)
                    return STATUS_OK;

                NSCursor* cursor = translate_cursor(pointer);
                if (!cursor)
                    return STATUS_UNKNOWN_ERR;

                // Commit the cursor value
                [cursor set]; // Sets the cursor immediately
                enPointer = pointer;

                return STATUS_OK;
            }

            mouse_pointer_t CocoaWindow::get_mouse_pointer()
            {
                return enPointer;
            } 

            status_t CocoaWindow::set_caption(const LSPString *caption)
            {
                if (caption == nullptr)
                    return STATUS_BAD_ARGUMENTS;
                if (pCocoaWindow == nullptr)
                    return STATUS_BAD_STATE;

                NSString *title = [NSString stringWithUTF8String:caption->get_utf8()];
                if (!title)
                    return STATUS_NO_MEM;

                [pCocoaWindow setTitle:title];
                return STATUS_OK;
            }

            status_t CocoaWindow::set_caption(const char *caption)
            {
                if (caption == nullptr)
                    return STATUS_BAD_ARGUMENTS;

                LSPString tmp;
                return (tmp.set_utf8(caption)) ? set_caption(&tmp) : STATUS_NO_MEM;
            }

            status_t CocoaWindow::get_caption(LSPString *text)
            {
                if (text == nullptr)
                    return STATUS_BAD_ARGUMENTS;
                if (pCocoaWindow == nullptr)
                    return STATUS_BAD_STATE;

                NSString *title = [pCocoaWindow title];
                const char *utf8 = [title UTF8String];
                return (text->set_utf8(utf8)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t CocoaWindow::get_caption(char *text, size_t len)
            {
                if (text == nullptr)
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
                return STATUS_OK;
            }

            //TODO: Translate all Window Border Styles!
            static NSWindowStyleMask get_ns_style(border_style_t style) {
                using namespace lsp::ws;
                switch (style) {
                    case BS_DIALOG:         return NSWindowStyleMaskTitled;
                    case BS_SINGLE:         return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
                    case BS_NONE:           return NSWindowStyleMaskBorderless;
                    /*
                    case BS_POPUP:
                    case BS_COMBO:
                    */
                    case BS_DROPDOWN:       return NSWindowStyleMaskBorderless;
                    case BS_SIZEABLE:       return NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                                NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
                    default:                return NSWindowStyleMaskTitled;
                }
            }

            //TODO: Translate all Window Border Styles!
            status_t CocoaWindow::get_border_style(border_style_t *style) {
                using namespace lsp::ws;

                if (style == NULL)
                    return STATUS_BAD_ARGUMENTS;

                if (pCocoaWindow == NULL)
                    return STATUS_BAD_STATE;

                NSWindowStyleMask windowStyle = [pCocoaWindow styleMask];

                if (windowStyle & NSWindowStyleMaskResizable)
                    *style = BS_SIZEABLE;

                if ((windowStyle & NSWindowStyleMaskTitled) &&
                    (windowStyle & NSWindowStyleMaskClosable) &&
                    (windowStyle & NSWindowStyleMaskMiniaturizable))
                    *style = BS_SINGLE;

                if (windowStyle & NSWindowStyleMaskTitled)
                    *style = BS_DIALOG;

                if (windowStyle == NSWindowStyleMaskBorderless)
                    *style = BS_NONE;

                // You may differentiate further if needed
                return STATUS_OK;;
            }

            status_t CocoaWindow::set_border_style(border_style_t style) {
                if (!pCocoaWindow)
                    return STATUS_BAD_STATE;

                NSWindowStyleMask desiredStyle = get_ns_style(style);

                // Only recreate if style has changed
                if ([pCocoaWindow styleMask] != desiredStyle) {
                    NSRect frame = [pCocoaWindow frame];
                    NSBackingStoreType backing = [pCocoaWindow backingType];
                    NSString *title = [pCocoaWindow title];
                    id delegate = [pCocoaWindow delegate];
                    BOOL isVisible = [pCocoaWindow isVisible];

                    [pCocoaWindow close];  // Dispose the old window

                    pCocoaWindow = [[NSWindow alloc] initWithContentRect:frame
                                                        styleMask:desiredStyle
                                                            backing:backing
                                                            defer:NO];
                    [pCocoaWindow setTitle:title];
                    [pCocoaWindow setDelegate:delegate];

                    [pCocoaWindow setContentView:pCocoaView];

                    if (isVisible) {
                        [pCocoaWindow makeKeyAndOrderFront:nil];
                    }
                }

                return STATUS_OK;
            }

            status_t CocoaWindow::set_window_actions(size_t actions)
            {
                if (pCocoaWindow == NULL)
                {
                    nActions = actions;
                    return STATUS_OK;
                }

                return (nActions != actions) ? commit_border_style(enBorderStyle, actions) : STATUS_OK;
            }

            status_t CocoaWindow::get_window_actions(size_t *actions)
            {
                if (actions == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (pCocoaWindow == NULL)
                    return STATUS_BAD_STATE;

                *actions    = nActions;
                return STATUS_OK;
            }

            status_t CocoaWindow::resize(ssize_t width, ssize_t height) {
                rectangle_t rect = sSize;
                rect.nWidth = width;
                rect.nHeight = height;
                return set_geometry(&rect);
            }

            void CocoaWindow::apply_constraints(rectangle_t *dst, const rectangle_t *req) {
                *dst = *req;

                if ((sConstraints.nMaxWidth >= 0) && (dst->nWidth > sConstraints.nMaxWidth))
                    dst->nWidth = sConstraints.nMaxWidth;
                if ((sConstraints.nMaxHeight >= 0) && (dst->nHeight > sConstraints.nMaxHeight))
                    dst->nHeight = sConstraints.nMaxHeight;
                if ((sConstraints.nMinWidth >= 0) && (dst->nWidth < sConstraints.nMinWidth))
                    dst->nWidth = sConstraints.nMinWidth;
                if ((sConstraints.nMinHeight >= 0) && (dst->nHeight < sConstraints.nMinHeight))
                    dst->nHeight = sConstraints.nMinHeight;
            }

            status_t CocoaWindow::set_geometry(const rectangle_t *realize) {
                if (!pCocoaWindow)
                    return STATUS_BAD_STATE;

                rectangle_t old = sSize;
                apply_constraints(&sSize, realize);

                if ((old.nLeft == sSize.nLeft) &&
                    (old.nTop == sSize.nTop) &&
                    (old.nWidth == sSize.nWidth) &&
                    (old.nHeight == sSize.nHeight))
                        return STATUS_OK;
                
                return set_geometry_impl();
            }

            status_t CocoaWindow::set_geometry_impl() {
                if (!pCocoaWindow)
                    return STATUS_BAD_STATE;

                // Calculate the frame rect from the content rect
                printf("Resize / move window {nL=%d, nT=%d, nW=%d, nH=%d}\n", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));
                NSRect contentRect = NSMakeRect(sSize.nLeft, sSize.nTop, sSize.nWidth, sSize.nHeight);
                NSRect frameRect = [pCocoaWindow frameRectForContentRect:contentRect];

                
                // macOS coordinates start from bottom-left, so we may need to flip Y if using a fixed origin
                NSScreen* screen = [pCocoaWindow screen];
                if (screen) {
                    NSRect frame = [screen frame];
                    CGFloat screenHeight = frame.size.height;
                    frameRect.origin.y = screenHeight - frameRect.origin.y - frameRect.size.height;
                }

                [pCocoaWindow setFrame:frameRect display:YES animate:NO];
                return STATUS_OK;
            }

            status_t CocoaWindow::show()
            {
                
                printf("Show window this=%p, window=%p, position={l=%d, t=%d, w=%d, h=%d} \n",
                    this, "pCocoaWindow",
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                commit_border_style(enBorderStyle, nActions);
                transientParent = nil;

                if (!has_parent()) {
                    NSRect frame = NSMakeRect(sSize.nLeft, sSize.nTop, sSize.nWidth, sSize.nHeight);
                    [pCocoaWindow setFrame:frame display:NO];
                } 

                return STATUS_OK;
            }

            status_t CocoaWindow::show(IWindow *over)
            {
                printf("Show window this=%p, window=%p, position={l=%d, t=%d, w=%d, h=%d}, over=%p \n",
                    this, pCocoaWindow,
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight),
                    over);

                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                transientParent = nil;

                if (over != nullptr) {
                    transientParent = (__bridge NSWindow *)(over->handle());
                    [pCocoaWindow setLevel:NSFloatingWindowLevel];
                    [pCocoaWindow setParentWindow:transientParent];
                }

                if (!has_parent()) {
                    place_above(transientParent);
                }

                [pCocoaWindow makeKeyAndOrderFront:nil];

                return STATUS_OK;
            }

            void CocoaWindow::place_above(NSWindow *parent) {
                if (!pCocoaWindow || !parent)
                    return;

                [parent addChildWindow:pCocoaWindow ordered:NSWindowAbove];
                [pCocoaWindow makeKeyAndOrderFront:nil];
            }

            //TODO: Map all needed styles
            status_t CocoaWindow::commit_border_style(border_style_t bs, size_t wa)
            {
                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                NSUInteger styleMask = NSWindowStyleMaskTitled;

                if (has_parent())
                {
                    // Child windows typically don't need special style on macOS
                    // Possibly NSBorderlessWindowMask if it's not intended to be interactive
                    styleMask = NSWindowStyleMaskBorderless;
                }
                else
                {
                    switch (bs)
                    {
                        case BS_DIALOG:
                            styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
                            break;

                        case BS_SINGLE:
                            styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
                            if (wa & WA_MINIMIZE)
                                styleMask |= NSWindowStyleMaskMiniaturizable;
                            if (wa & WA_MAXIMIZE)
                                styleMask |= NSWindowStyleMaskResizable;
                            break;

                        case BS_SIZEABLE:
                            styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
                            if (wa & WA_MINIMIZE)
                                styleMask |= NSWindowStyleMaskMiniaturizable;
                            if (wa & WA_MAXIMIZE)
                                styleMask |= NSWindowStyleMaskResizable;
                            break;

                        case BS_POPUP:
                        case BS_COMBO:
                        case BS_DROPDOWN:
                            styleMask = NSWindowStyleMaskBorderless;
                            [pCocoaWindow setLevel:NSFloatingWindowLevel];
                            break;

                        case BS_NONE:
                        default:
                            styleMask = NSWindowStyleMaskBorderless;
                            break;
                    }
                }

                [pCocoaWindow setStyleMask:styleMask];

                return STATUS_OK;
            }

            status_t CocoaWindow::get_geometry(rectangle_t *realize)
            {
                if (realize == NULL)
                    return STATUS_BAD_ARGUMENTS;

                *realize            = sSize;

                return STATUS_OK;
            }

            status_t CocoaWindow::get_absolute_geometry(rectangle_t *realize)
            {
                if (realize == nullptr)
                    return STATUS_BAD_ARGUMENTS;
                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                NSRect frame = [pCocoaWindow frame]; // Frame in screen coordinates

                realize->nLeft   = static_cast<ssize_t>(frame.origin.x);
                realize->nTop    = static_cast<ssize_t>(frame.origin.y);
                realize->nWidth  = static_cast<ssize_t>(frame.size.width);
                realize->nHeight = static_cast<ssize_t>(frame.size.height);

                return STATUS_OK;
            }

            //TODO: we need to map all events, and handle create_surface
            status_t CocoaWindow::handle_event(const event_t *ev)
            {
                // Additionally generated event
                event_t gen;
                gen.nType = UIE_UNKNOWN;

                IEventHandler *handler = pHandler;

                //TODO: Create new surface on any event - just for testing
                drop_surface();
                pSurface = create_surface(pCocoaDisplay, sSize.nWidth, sSize.nHeight);

                switch (ev->nType)
                {
                    case UIE_SHOW:
                    {
                        bVisible = true;
                        if (bWrapper) break;

                        drop_surface();

                        //pSurface = create_surface(pCocoaDisplay, sSize.nWidth, sSize.nHeight);
                        break;
                    }

                    case UIE_HIDE:
                    {
                        bVisible = false;
                        if (bWrapper) break;

                        drop_surface();
                        break;
                    }

                    case UIE_REDRAW:
                    {
                        //TODO: Redraw logic 
                        break;
                    }

                    case UIE_SIZE_REQUEST:
                    {
                        //TODO: Resize logic
                        break;
                    }

                    case UIE_RESIZE:
                    {
                        if (bWrapper) break;

                        sSize.nLeft = ev->nLeft;
                        sSize.nTop = ev->nTop;
                        sSize.nWidth = ev->nWidth;
                        sSize.nHeight = ev->nHeight;

                        if (pSurface != nullptr)
                            pSurface->resize(sSize.nWidth, sSize.nHeight);
                        break;
                    }

                    case UIE_MOUSE_MOVE:
                    {
                        //TODO: Mouse movement logic 
                        printf("Mouse moved in window! \n");
                        break;
                    }

                    case UIE_MOUSE_DOWN:
                    {
                        vBtnEvent[0] = vBtnEvent[1];
                        vBtnEvent[1] = vBtnEvent[2];
                        vBtnEvent[2].sDown = *ev;
                        init_event(&vBtnEvent[2].sUp);
                        break;
                    }

                    case UIE_MOUSE_UP:
                    {
                        vBtnEvent[2].sUp = *ev;

                        if (check_click(&vBtnEvent[2]))
                        {
                            gen = *ev;
                            gen.nType = UIE_MOUSE_CLICK;

                            if (check_double_click(&vBtnEvent[1], &vBtnEvent[2]))
                            {
                                gen.nType = UIE_MOUSE_DBL_CLICK;

                                if (check_double_click(&vBtnEvent[0], &vBtnEvent[1]))
                                    gen.nType = UIE_MOUSE_TRI_CLICK;
                            }
                        }
                        break;
                    }

                    case UIE_STATE:
                    {
                        if (ev->nCode == enState)
                            return STATUS_OK;

                        enState = static_cast<window_state_t>(ev->nCode);
                        lsp_trace("Window state changed to: %d", int(ev->nCode));
                        break;
                    }

                    case UIE_CLOSE:
                    {
                        if (handler == nullptr)
                        {
                            this->destroy();
                            delete this;
                        }
                        break;
                    }

                    default:
                        break;
                }

                // Pass event to handler
                if (handler != nullptr)
                {
                    handler->handle_event(ev);
                    if (gen.nType != UIE_UNKNOWN)
                        handler->handle_event(&gen);
                }

                return STATUS_OK;
            }

            bool CocoaWindow::check_click(const btn_event_t *ev)
            {
                if ((ev->sDown.nType != UIE_MOUSE_DOWN) || (ev->sUp.nType != UIE_MOUSE_UP))
                    return false;
                if (ev->sDown.nCode != ev->sUp.nCode)
                    return false;
                if ((ev->sUp.nTime < ev->sDown.nTime) || ((ev->sUp.nTime - ev->sDown.nTime) > 400))
                    return false;

                return (ev->sDown.nLeft == ev->sUp.nLeft) && (ev->sDown.nTop == ev->sUp.nTop);
            }

            bool CocoaWindow::check_double_click(const btn_event_t *pe, const btn_event_t *ev)
            {
                if (!check_click(pe))
                    return false;

                if (pe->sDown.nCode != ev->sDown.nCode)
                    return false;
                if ((ev->sUp.nTime < pe->sUp.nTime) || ((ev->sUp.nTime - pe->sUp.nTime) > 400))
                    return false;

                return (pe->sUp.nLeft == ev->sUp.nLeft) && (pe->sUp.nTop == ev->sUp.nTop);
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_MACOSX */


