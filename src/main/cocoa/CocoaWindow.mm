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


#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#import <Cocoa/Cocoa.h>

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

#include <cairo.h>
#include <cairo-quartz.h>

#include <thread>
#include <chrono>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            
            CocoaWindow::CocoaWindow(CocoaDisplay *dpy, NSView *view, IEventHandler *handler, bool wrapper): ws::IWindow(dpy, handler)
            {
                lsp_trace("%zu", wrapper);

                pCocoaDisplay = dpy;
               
                nActions                = WA_SINGLE;
                enPointer               = MP_DEFAULT;
                enState                 = WS_NORMAL;

                bWrapper                = wrapper;

                if (bWrapper) {
                    pCocoaWindow = [view window];
                }

                sSize.nLeft             = 0;
                sSize.nTop              = 0;
                sSize.nWidth            = 320;
                sSize.nHeight           = 320;

                sConstraints.nMinWidth  = -1;
                sConstraints.nMinHeight = -1;
                sConstraints.nMaxWidth  = -1;
                sConstraints.nMaxHeight = -1;
                sConstraints.nPreWidth  = -1;
                sConstraints.nPreHeight = -1;

                windowObserverTokens = [[NSMutableArray alloc] init];
                viewObserverTokens = [[NSMutableArray alloc] init];
            }

            CocoaWindow::~CocoaWindow()
            {
                pCocoaDisplay   = NULL;
                pCocoaWindow    = NULL;
            }

            status_t CocoaWindow::init()
            {

                lsp_trace("Window init!");
                // Initialize parent class
                status_t res = IWindow::init();
                if (res != STATUS_OK)
                    return res;

                if (!bWrapper) {
                    ssize_t screenWidth, screenHeight;
                    pCocoaDisplay->screen_size(0, &screenWidth, &screenHeight);
                    NSRect frame = NSMakeRect(sSize.nLeft, screenHeight - sSize.nTop - sSize.nHeight + pCocoaDisplay->get_window_title_height(), sSize.nWidth, sSize.nHeight + pCocoaDisplay->get_window_title_height());    

                    // Create a window
                    NSWindow *window = [[NSWindow alloc]
                                                initWithContentRect:frame
                                                styleMask:(NSWindowStyleMaskTitled |
                                                            NSWindowStyleMaskClosable |
                                                            NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                defer:NO];

                    pCocoaWindow = window;
                    [pCocoaWindow setIsVisible:NO];

                    // Create a cocoa view and set it to window
                    CocoaCairoView *view = [[CocoaCairoView alloc] initWithFrame:frame];
                    view.display = pCocoaDisplay;
                    pCocoaView = view;
                    [pCocoaWindow setContentView:pCocoaView];
                    [pCocoaView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
                    [pCocoaView registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
                    [pCocoaWindow makeFirstResponder:pCocoaView];
                    
                    set_border_style(BS_SIZEABLE);
                    set_window_actions(WA_ALL);
                    init_notification_center(pCocoaWindow);
                    init_notification_center(pCocoaView);
                } else {
                    CocoaCairoView *wrapperView = [[CocoaCairoView alloc] initWithFrame:[[pCocoaWindow contentView] bounds]];
                    [[pCocoaWindow contentView] addSubview:wrapperView positioned:NSWindowAbove relativeTo:nil];
                    wrapperView.display = pCocoaDisplay;
                    pCocoaView = wrapperView;
                    [pCocoaView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
                    [pCocoaWindow makeFirstResponder:pCocoaView];
                    
                    init_notification_center(pCocoaView);
                }

                
                set_mouse_pointer(MP_DEFAULT);
                bInvalidate = true;

                return STATUS_OK;
            }

            void CocoaWindow::init_notification_center(NSWindow *window) {
                NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

                id didBecomeKeyNotificationToken = [center addObserverForName:NSWindowDidBecomeKeyNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_FOCUS_IN");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_FOCUS_IN;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:didBecomeKeyNotificationToken];

                id didResignKeyNotificationToken = [center addObserverForName:NSWindowDidResignKeyNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_FOCUS_OUT");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_FOCUS_OUT;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:didResignKeyNotificationToken];

                id didMiniaturizeNotificationToken = [center addObserverForName:NSWindowDidMiniaturizeNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_HIDE");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_HIDE;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:didMiniaturizeNotificationToken];

                id didDeminiaturizeNotificationToken = [center addObserverForName:NSWindowDidDeminiaturizeNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_SHOW");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_SHOW;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:didDeminiaturizeNotificationToken];

                id willCloseNotificationToken = [center addObserverForName:NSWindowWillCloseNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_CLOSE");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_CLOSE;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:willCloseNotificationToken];

                id dragEnterToken = [center addObserverForName:@"DragEnter"
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_DRAG_ENTER");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_DRAG_ENTER;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:dragEnterToken];

                id dragExitToken = [center addObserverForName:@"DragExit"
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        lsp_trace("UIE_DRAG_LEAVE");
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_DRAG_LEAVE;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:dragExitToken];

                //TODO: implement all notification events
                
                id didResizeNotificationToken = [center addObserverForName:NSWindowDidResizeNotification
                                    object:window
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        ssize_t screenWidth, screenHeight;
                                        pCocoaDisplay->screen_size(0, &screenWidth, &screenHeight);
                                        NSWindow *nsWindow = note.object;
                                        NSRect wFrame = [nsWindow frame];
                                        NSRect cFrame = [[nsWindow contentView] frame];
                                        event_t ue;
                                        init_event(&ue);
                                        ue.nType       = UIE_RESIZE;
                                        ue.nTop        = screenHeight - wFrame.origin.y - cFrame.size.height;
                                        ue.nLeft       = wFrame.origin.x;
                                        ue.nWidth      = cFrame.size.width;
                                        ue.nHeight     = cFrame.size.height;
                                        handle_event(&ue);
                                    }];
                [windowObserverTokens addObject:didResizeNotificationToken];
            }

            void CocoaWindow::init_notification_center(CocoaCairoView *view) {
                NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

                id redrawRequestToken = [center addObserverForName:@"RedrawRequest"
                                    object:view
                                    queue:[NSOperationQueue mainQueue]
                                    usingBlock:^(NSNotification *note) {
                                        if (bInvalidate) {
                                            event_t ue;
                                            init_event(&ue);
                                            ue.nType       = UIE_REDRAW;
                                            handle_event(&ue);
                                            bInvalidate = false;
                                        }
                                    }];
                [viewObserverTokens addObject:redrawRequestToken];

            }

            void CocoaWindow::destroy()
            {
                if (bWrapper) {
                    for (id token in viewObserverTokens) {
                        [[NSNotificationCenter defaultCenter] removeObserver:token];
                    }
                    [[NSNotificationCenter defaultCenter] removeObserver:pCocoaView];
                    [viewObserverTokens removeAllObjects];
                }
                else
                {
                    for (id token in windowObserverTokens) {
                        [[NSNotificationCenter defaultCenter] removeObserver:token];
                    }
                    [[NSNotificationCenter defaultCenter] removeObserver:pCocoaWindow];
                    [windowObserverTokens removeAllObjects];
                }

                if ([pCocoaView superview]) {
                    [pCocoaView removeFromSuperview];
                    [pCocoaView release];
                    pCocoaView = NULL;
                }
                    
                hide();
                drop_surface();

                // Surface related
                if (pSurface != NULL)
                {
                    pSurface->destroy();
                    delete pSurface;
                    pSurface = NULL;
                }

                if (pCocoaDisplay != NULL)  
                    pCocoaDisplay->vWindows.qpremove(this);
                
                if (!bWrapper && pCocoaWindow != NULL)
                    [pCocoaWindow close];
                
                pCocoaDisplay = NULL;
                IWindow::destroy();
            }

            NSWindow *CocoaWindow::nswindow() const {
                return pCocoaWindow;
            }

            ISurface *CocoaWindow::create_surface(CocoaDisplay *display, CocoaCairoView *view, size_t width, size_t height)
            {
                ISurface *result = NULL;

                if (result == NULL)
                {
                    
                    result = new CocoaCairoSurface(display, view, width, height);
                    if (result != NULL)
                        lsp_trace("Using CocoaCairoSurface ptr=%p", result);
                }
                return result;
            }

            // Triggers redraw from app and tests
            status_t CocoaWindow::invalidate()
            {
                if (pSurface == NULL)
                    return STATUS_BAD_STATE;

                bInvalidate = true;

                
                event_t ue;
                init_event(&ue);

                ue.nType       = UIE_REDRAW;
                ue.nLeft       = sSize.nLeft;
                ue.nTop        = sSize.nHeight;
                ue.nWidth      = sSize.nWidth;
                ue.nHeight     = sSize.nHeight;

                handle_event(&ue);
               
/*
                NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
                [center postNotificationName:@"RedrawRequest"
                                    object:pCocoaWindow];
*/
                //Trigger CocoaCairoView to redraw!
                //[pCocoaView setCursor: pCocoaCursor];
                //[pCocoaView setImage: get_image_surface()];
                //[pCocoaView triggerRedraw];
                
                return STATUS_OK;
            }


            ISurface *CocoaWindow::get_surface()
            {
                /*
                if(bWrapper)
                    return NULL;
                */
                if (pSurface != NULL)
                    return pSurface;

                return NULL;
            }

            cairo_surface_t *CocoaWindow::get_image_surface()
            {
                if (pSurface != NULL) {
                    return reinterpret_cast<CocoaCairoSurface *>(pSurface)->get_image_surface();
                }

                return NULL;
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
                    case MP_ARROW:        return [NSCursor arrowCursor];
                    /* TODO: implement custom cursor?
                    MP_ARROW_LEFT,      // Arrow left
                    MP_ARROW_RIGHT,     // Arrow right
                    MP_ARROW_UP,        // Arrow up
                    MP_ARROW_DOWN,      // Arrow down
                    MP_DRAW,            // Drawing tool (pencil)
                    MP_PLUS,            // Plus
                    MP_SIZE,            // Size
                    MP_SIZE_NESW,       // Sizing cursor oriented diagonally from northeast to southwest
                    MP_SIZE_NWSE,       // Sizing cursor oriented diagonally from northwest to southeast
                    MP_UP_ARROW,        // Arrow pointing up
                    MP_HOURGLASS,       // Hourglass
                    MP_DRAG,            // Arrow with a blank page in the lower-right corner
                    MP_HSPLIT,          // Black double-vertical bar with arrows pointing right and left
                    MP_VSPLIT,          // Black double-horizontal bar with arrows pointing up and down
                    MP_MULTIDRAG,       // Arrow with three blank pages in the lower-right corner
                    MP_APP_START,       // Arrow combined with an hourglass
                    MP_HELP,            // Arrow next to a black question mark
                    */
                    case MP_DRAG:         return [NSCursor closedHandCursor];
                    case MP_DANGER:
                    case MP_NO_DROP:      return [NSCursor operationNotAllowedCursor];
                    case MP_IBEAM:        return [NSCursor IBeamCursor];
                    case MP_PLUS:
                    case MP_CROSS:        return [NSCursor crosshairCursor];
                    case MP_HAND:         return [NSCursor pointingHandCursor];
                    case MP_SIZE_NS:      return [NSCursor resizeUpDownCursor];
                    case MP_SIZE_WE:      return [NSCursor resizeLeftRightCursor];
                    default:              return [NSCursor arrowCursor]; // Fallback to default cursor
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

                pCocoaCursor = cursor;
                enPointer = pointer;

                [pCocoaView setCursor: pCocoaCursor];

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

                NSString *title = [NSString stringWithCharacters:reinterpret_cast<const unichar *>(caption->get_utf16())
                                            length: caption->length()];
                if (!title)
                    return STATUS_NO_MEM;

                lsp_trace("%s, %p", caption->get_utf8(), pCocoaWindow);
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
            NSWindowStyleMask CocoaWindow::get_ns_style(border_style_t style, size_t wa) {
                NSUInteger styleMask = NSWindowStyleMaskTitled;

                if (has_parent())
                {
                    // Child windows typically don't need special style on macOS
                    // Possibly NSBorderlessWindowMask if it's not intended to be interactive
                    styleMask = NSWindowStyleMaskBorderless;
                }
                else
                {
                    switch (style)
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

                return styleMask;
            }

            status_t CocoaWindow::get_border_style(border_style_t *style) {
                using namespace lsp::ws;

                if (style == NULL)
                    return STATUS_BAD_ARGUMENTS;
                if (pCocoaWindow == NULL)
                    return STATUS_BAD_STATE;

                *style    = enBorderStyle;
                return STATUS_OK;
            }

            status_t CocoaWindow::set_border_style(border_style_t style) {
                if (!pCocoaWindow)
                    return STATUS_BAD_STATE;

                enBorderStyle = style;
                commit_border_style(enBorderStyle, nActions);

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

            NSWindow* CocoaWindow::get_window_handler() 
            {
                return pCocoaWindow;
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

                lsp_trace("minW=%d, minH=%d, maxW=%d, maxH=%d", int(sConstraints.nMinWidth), int(sConstraints.nMinHeight), int(sConstraints.nMaxWidth), int(sConstraints.nMaxHeight));
            }

            status_t CocoaWindow::set_size_constraints(const size_limit_t *c)
            {
                sConstraints    = *c;
                if (sConstraints.nMinWidth == 0)
                    sConstraints.nMinWidth  = 1;
                if (sConstraints.nMinHeight == 0)
                    sConstraints.nMinHeight = 1;

                // Apply constrains to Cocoa Window
                if ([pCocoaView window] != NULL) {
                    [[pCocoaView window] setContentMinSize:NSMakeSize(sConstraints.nMinWidth, sConstraints.nMinHeight)];
                    [[pCocoaView window] setContentMaxSize:NSMakeSize(sConstraints.nMaxWidth, sConstraints.nMaxHeight)];
                }
                lsp_trace("constrained: l=%d, t=%d, w=%d, h=%d", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                return set_geometry(&sSize);
            }

            status_t CocoaWindow::get_size_constraints(size_limit_t *c)
            {
                *c = sConstraints;
                return STATUS_OK;
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
                if (![pCocoaView window])
                    return STATUS_BAD_STATE;

                // Calculate the frame rect from the content rect
                lsp_trace("Resize / move window {nL=%d, nT=%d, nW=%d, nH=%d}\n", int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));
                ssize_t screenWidth, screenHeight;
                pCocoaDisplay->screen_size(0, &screenWidth, &screenHeight);
                if ([pCocoaView window] != NULL) {
                    //NSRect currentFrame = [[pCocoaView window] frame];
                    NSRect contentRect = NSMakeRect(sSize.nLeft, screenHeight - sSize.nTop - sSize.nHeight + pCocoaDisplay->get_window_title_height(), sSize.nWidth, sSize.nHeight);
                    NSRect frameRect = [[pCocoaView window] frameRectForContentRect:contentRect];

                    [[pCocoaView window] setFrame:frameRect display:YES animate:NO];
                }
                return STATUS_OK;
            }

            status_t CocoaWindow::show()
            {
                
                lsp_trace("Show window this=%p, window=%p, position={l=%d, t=%d, w=%d, h=%d} \n",
                    this, "pCocoaWindow",
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight));

                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                commit_border_style(enBorderStyle, nActions);
                transientParent = nil;

                if (!has_parent()) {
                    lsp_trace("!has_parent");
                    ssize_t screenWidth, screenHeight;
                    pCocoaDisplay->screen_size(0, &screenWidth, &screenHeight);
                    NSRect frame = NSMakeRect(sSize.nLeft, screenHeight - sSize.nTop - sSize.nHeight + pCocoaDisplay->get_window_title_height(), sSize.nWidth, sSize.nHeight + pCocoaDisplay->get_window_title_height());
                    [pCocoaWindow setFrame:frame display:NO];
                } 

                if (bWrapper && pCocoaView && pCocoaWindow) {
                    lsp_trace("bWrapper && pCocoaView && pCocoaWindow");
                    NSRect contentRect = [[pCocoaWindow contentView] bounds];
                    [pCocoaView setFrame:contentRect];
                }

                [pCocoaWindow makeKeyAndOrderFront:nil];

                // Simulate missing show event
                event_t ue;
                init_event(&ue);
                ue.nType       = UIE_SHOW;
                handle_event(&ue);

                return STATUS_OK;
            }

            status_t CocoaWindow::show(IWindow *over)
            {
                lsp_trace("Show window this=%p, window=%p, position={l=%d, t=%d, w=%d, h=%d}, over=%p \n",
                    this, pCocoaWindow,
                    int(sSize.nLeft), int(sSize.nTop), int(sSize.nWidth), int(sSize.nHeight),
                    over);

                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                transientParent = nil;

                if (over != nullptr) {
                    NSView *view = (__bridge NSView *)(over->handle());
                    transientParent = [view window];
                    [pCocoaWindow setLevel:NSFloatingWindowLevel];
                    [pCocoaWindow setParentWindow:transientParent];
                }

                if (!has_parent()) {
                    place_above(transientParent);
                }

                [pCocoaWindow makeKeyAndOrderFront:nil];

                // Simulate missing show event
                event_t ue;
                init_event(&ue);
                ue.nType       = UIE_SHOW;
                handle_event(&ue);

                return STATUS_OK;
            }

            void CocoaWindow::place_above(NSWindow *parent) {
                lsp_trace("place_above");

                if (!pCocoaWindow || !parent)
                    return;

                [parent addChildWindow:pCocoaWindow ordered:NSWindowAbove];
/*
                NSRect parentFrame = [parent frame];
                NSRect childFrame = [pCocoaWindow frame];
                NSRect viewFrameInWindow = [[pCocoaWindow contentView] frame];
                NSRect viewFrameOnScreen = [[pCocoaWindow contentView] convertRect:viewFrameInWindow toView:nil];

                // Or for a point:
                NSPoint originOnScreen = [view convertPoint:viewFrameInWindow.origin toView:nil];
*/
                /*
                NSRect parentFrame = [parent frame]; // bottom-left based
                CGFloat parentTopY = parentFrame.origin.y + parentFrame.size.height; // top edge in screen coords

                // Suppose you want to place the child window at (offsetX, offsetY) from the parent's top-left
                CGFloat childY = parentTopY - sSize.nTop - childFrame.size.height; // convert to bottom-left
                */
                
                lsp_trace("Show pos position={l=%d, t=%d,} \n", int(sSize.nLeft), int(sSize.nTop));
                NSPoint origin = NSMakePoint(sSize.nLeft, sSize.nTop);
                [pCocoaWindow setFrameOrigin:origin];
                //[pCocoaWindow makeKeyAndOrderFront:nil];
            }

            status_t CocoaWindow::hide()
            {
                bVisible        = false;
                if(pCocoaWindow != NULL) {
                    [pCocoaWindow orderOut:nil];
                }

                return STATUS_OK;
            }

            bool CocoaWindow::is_visible()
            {
                return bVisible;
            }

            status_t CocoaWindow::commit_border_style(border_style_t bs, size_t wa)
            {
                if (pCocoaWindow == nil)
                    return STATUS_BAD_STATE;

                NSWindowStyleMask desiredStyle = get_ns_style(bs, wa);
                [pCocoaWindow setStyleMask:desiredStyle];

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
                NSRect cFrame = [[pCocoaWindow contentView] frame];

                realize->nLeft   = static_cast<ssize_t>(frame.origin.x);
                realize->nTop    = static_cast<ssize_t>(frame.origin.y);
                realize->nWidth  = static_cast<ssize_t>(cFrame.size.width);
                realize->nHeight = static_cast<ssize_t>(cFrame.size.height);

                return STATUS_OK;
            }

            void *CocoaWindow::handle()
            {   
                return (__bridge void*)pCocoaView;
            }

            //TODO: we need to map all events, and handle create_surface
            status_t CocoaWindow::handle_event(const event_t *ev)
            {
                // Additionally generated event
                event_t gen;
                init_event(&gen);
                gen.nType = UIE_UNKNOWN;

                IEventHandler *handler = pHandler;

                switch (ev->nType)
                {
                    case UIE_SHOW:
                    {
                        bVisible = true;
                        //if (bWrapper) break;

                        drop_surface();
                        pSurface = create_surface(pCocoaDisplay, pCocoaView, sSize.nWidth, sSize.nHeight);

                        [pCocoaView startRedrawLoop];
                        break;
                    }

                    case UIE_HIDE:
                    {
                        bVisible = false;
                        //if (bWrapper) break;

                        [pCocoaView stopRedrawLoop];
                        drop_surface();
                        break;
                    }

                    case UIE_REDRAW:
                    {
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
                            gen.nLeft = ev->nLeft;
                            gen.nTop =  ev->nTop;

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

            ssize_t CocoaWindow::left()
            {
                return sSize.nLeft;
            }

            ssize_t CocoaWindow::top()
            {
                return sSize.nTop;
            }

            ssize_t CocoaWindow::width()
            {
                return sSize.nWidth;
            }

            ssize_t CocoaWindow::height()
            {
                return sSize.nHeight;
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_MACOSX */


