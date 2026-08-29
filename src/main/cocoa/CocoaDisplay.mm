/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/ws/types.h>
#include <lsp-plug.in/ws/keycodes.h>
#include <lsp-plug.in/ws/cocoa/decode.h>

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/cocoa/CocoaDisplay.h>
#include <private/cocoa/CocoaWindow.h>
#include <private/cocoa/CocoaCairoView.h>
#include <private/cocoa/defs.h>

// Forward-target for the 60 Hz redraw NSTimer that drives
// CocoaDisplay::do_main_iteration in hosted mode. Same shape as
// LSPRedrawTimerProxy in CocoaCairoView.mm: NSTimer retains its target,
// so we keep the back-pointer raw and let the C++ destroy() clear it
// before the display is freed. Block-based NSTimer was rejected because
// the captured block could outlive the C++ destroy() call when the run
// loop released the timer asynchronously.
@interface LSPDisplayTimerProxy : NSObject {
    lsp::ws::cocoa::CocoaDisplay *_display;
}
- (instancetype)initWithDisplay:(lsp::ws::cocoa::CocoaDisplay *)display;
- (void)invalidate;
- (void)tick:(NSTimer *)timer;
@end

@implementation LSPDisplayTimerProxy
- (instancetype)initWithDisplay:(lsp::ws::cocoa::CocoaDisplay *)display
{
    self = [super init];
    if (self)
        _display = display;
    return self;
}
- (void)invalidate
{
    _display = NULL;
}
- (void)tick:(NSTimer *)timer
{
    if (_display != NULL)
        _display->tick_redraw();
}
@end

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            CocoaDisplay::CocoaDisplay(): IDisplay()
            {
               bExit                   = false;
               lastMouseButton         = 0;
               pDragTarget             = NULL;
               pGrabMonitor            = NULL;
               pIterationTimer         = NULL;
               pIterationTimerProxy    = NULL;
            }

            CocoaDisplay::~CocoaDisplay()
            {
            }

            status_t CocoaDisplay::init(int argc, const char **argv)
            {
                if (NSApp == NULL)
                {
                    [NSApplication sharedApplication];
                    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                    [NSApp activateIgnoringOtherApps:YES];
                    standaloneApp = true;
                }
                else 
                    standaloneApp = false;

                // Initialize font manager
            #ifdef USE_LIBFREETYPE
                {
                    status_t fm_res    = sFontManager.init();
                    if (fm_res != STATUS_OK)
                        return fm_res;
                }
            #endif /* USE_LIBFREETYPE */

                get_enviroment_frame_sizes();

                // Create estimation surface
                pEstimation     = new CocoaCairoSurface(this, 1, 1);
                if (pEstimation == NULL)
                    return STATUS_NO_MEM;

                // In hosted mode (plugin) NSApp is owned by the host. Install
                // a single 60 Hz NSTimer into the host's NSRunLoop that drives
                // do_main_iteration() for every registered window.
                // In standalone mode CocoaDisplay::main() runs its own loop, so
                // no timer is needed.
                if (!standaloneApp)
                {
                    LSPDisplayTimerProxy *proxy = [[LSPDisplayTimerProxy alloc] initWithDisplay:this];
                    NSTimer *timer = [NSTimer timerWithTimeInterval:(1.0/60.0)
                                              target:proxy
                                              selector:@selector(tick:)
                                              userInfo:nil
                                              repeats:YES];
                    // NSRunLoopCommonModes, not the default mode: during a live window
                    // resize (or menu tracking) the run loop switches to an event
                    // tracking mode where default-mode timers do not fire — the UI
                    // would stop redrawing for the whole duration of the drag.
                    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
                    pIterationTimer      = (void *) timer;   // owned by run loop
                    pIterationTimerProxy = (void *) proxy;   // owned by us
                }

                return IDisplay::init(argc, argv);
            }

            void CocoaDisplay::tick_redraw()
            {
                @autoreleasepool {
                    do_main_iteration(system::get_time_millis());
                    for (size_t i = 0, n = vWindows.size(); i < n; ++i)
                    {
                        CocoaWindow * const wnd = vWindows.uget(i);
                        if (wnd == NULL || wnd->pCocoaView == nil)
                            continue;
                        if (wnd->pCocoaView.needsRedrawing)
                            [wnd->pCocoaView setNeedsDisplay:YES];
                    }
                }
            }

            status_t CocoaDisplay::main()
            {
                // Initialize the main loop
                bExit = false;
                status_t res = process_pending_events();
                if (res != STATUS_OK)
                    return res;
                
                // Do the main loop
                while (!bExit)
                {
                    // Do one main iteration
                    const timestamp_t ts = system::get_time_millis();
                    if ((res = do_main_iteration(ts)) != STATUS_OK)
                        return res;
                    
                    // Wait for a while to not to raise CPU load
                    if ((res = wait_events(idle_interval())) != STATUS_OK)
                        return res;
                }
                
                // Process all currently pending events
                return process_pending_events();
            }

            void CocoaDisplay::get_enviroment_frame_sizes()
            {
                NSWindow *tempWindow = [[NSWindow alloc]  initWithContentRect:NSMakeRect(0,0,20,20)
                                                          styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                          backing:NSBackingStoreBuffered
                                                          defer:NO];

                // Get frame and content rect
                NSRect fRect = tempWindow.frame;
                NSRect cRect = [tempWindow contentRectForFrameRect:fRect];

                titleHeight = fRect.size.height - cRect.size.height;
                borderWidth = fRect.size.width - cRect.size.width;

                [tempWindow orderOut:nil];
                [tempWindow close];
                tempWindow = nil;  
            }

            size_t CocoaDisplay::get_window_title_height()
            {
                return titleHeight;
            }

            size_t CocoaDisplay::get_window_border_width()
            {
                return borderWidth;
            }

            ft::FontManager *CocoaDisplay::font_manager()
            {
            #ifdef USE_LIBFREETYPE
                return &sFontManager;
            #else
                return NULL;
            #endif /* USE_LIBFREETYPE */
            }

            status_t CocoaDisplay::do_main_iteration(timestamp_t ts)
            {
                // Here, any queued Cocoa events already handled via sendEvent.
                // Use this to do your own rendering / app logic.
                @autoreleasepool {
                    if (standaloneApp)
                    {
                        NSEvent *event;
                        while ((event = [NSApp  nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                inMode:NSDefaultRunLoopMode
                                                dequeue:YES]))
                        {
                            [NSApp sendEvent:event];
                            [NSApp updateWindows];
                        }
                    }

                    // Handle internal tasks
                    status_t result = process_pending_tasks(ts);

                #ifdef USE_LIBFREETYPE
                    sFontManager.gc();
                #endif
                    
                    // Redraw windows if they are invalidated
                    for (size_t i=0, n=vWindows.size(); i<n; ++i)
                    {
                        CocoaWindow * const wnd = vWindows.uget(i);
                        if (wnd != NULL)
                            wnd->redraw();
                    }

                    return result;
                }
            }

            bool CocoaDisplay::r3d_backend_supported(const r3d::backend_metadata_t *meta)
            {
                // CoocaDisplay display supports only offscreen 
                if (meta->wnd_type == r3d::WND_HANDLE_NONE)
                    return true;
                return IDisplay::r3d_backend_supported(meta);
            }

            void CocoaDisplay::handle_event(const nsevent_t & event)
            {
                const NSEvent * const nsevent = event.event;
                if (!nsevent)
                    return;

                NSEventType type = [nsevent type];

                // During an in-progress drag, route mouse events to the window that received
                // the matching mouseDown — even if the cursor leaves the view's bounds.
                const bool isDragMove = (type == NSEventTypeLeftMouseDragged) ||
                                        (type == NSEventTypeRightMouseDragged) ||
                                        (type == NSEventTypeOtherMouseDragged) ||
                                        (type == NSEventTypeMouseMoved);
                const bool isMouseUp  = (type == NSEventTypeLeftMouseUp) ||
                                        (type == NSEventTypeRightMouseUp) ||
                                        (type == NSEventTypeOtherMouseUp);

                CocoaWindow *target = NULL;
                if ((isDragMove || isMouseUp) && pDragTarget != NULL)
                    target = pDragTarget;

                const nswindow_t nsWindow = nswindow_t { [nsevent window] };
                if (!target)
                    target = find_window(nsWindow);

                // Embedded case: the NSEvent's window is the host's (e.g. Ableton's), not ours.
                // Locate our CocoaWindow by walking up from the hit-test view to a known pCocoaView.
                if (!target)
                {
                    NSView *root = [nsWindow.window contentView];
                    NSPoint p = [nsevent locationInWindow];
                    NSView *hit = [root hitTest:p];
                    for (size_t i = 0, n = vWindows.size(); i < n && !target; ++i)
                    {
                        CocoaWindow *w = vWindows.uget(i);
                        if (!w || !w->pCocoaView)
                            continue;
                        NSView *v = hit;
                        while (v != nil)
                        {
                            if (v == w->pCocoaView)
                            {
                                target = w;
                                break;
                            }
                            v = [v superview];
                        }
                    }
                }

                if (!target)
                    return;

                event_t ue = {};
                init_event(&ue);
                ue.nTime = timestamp_t([nsevent timestamp] * 1000);

                unsigned short keyCode = 65535;
                NSString *chars = @"";
                unichar keysym = 0;

                NSPoint locInWindow = [nsevent locationInWindow];
                // Resolve event coordinates against the target view directly so they stay valid
                // when the cursor leaves the view (during a drag).
                NSView *coordView = target->pCocoaView;
                NSPoint locInView;
                NSRect cFrame;
                if (coordView != nil && [coordView window] != nil)
                {
                    if ([coordView window] != nsWindow.window)
                    {
                        NSPoint scr = [nsWindow.window convertPointToScreen:locInWindow];
                        NSPoint inHostWnd = [[coordView window] convertPointFromScreen:scr];
                        locInView = [coordView convertPoint:inHostWnd fromView:nil];
                    }
                    else
                    {
                        locInView = [coordView convertPoint:locInWindow fromView:nil];
                    }
                    cFrame = [coordView frame];
                }
                else
                {
                    NSView *hitView = [[nsWindow.window contentView] hitTest:locInWindow];
                    locInView = [hitView convertPoint:locInWindow fromView:nil];
                    cFrame = [hitView frame];
                }

                ue.nLeft = locInView.x;
                ue.nTop = cFrame.size.height - locInView.y;
                //TODO: Is there any problem when the mouse nTop state can negativ?

                switch (type)
                {
                    case NSEventTypeLeftMouseDown:
                    case NSEventTypeRightMouseDown:
                    case NSEventTypeOtherMouseDown:
                        ue.nType = UIE_MOUSE_DOWN;
                        ue.nCode = decode_mcb(nsevent);
                        lastMouseButton = decode_modifier(nsevent);
                        pDragTarget = target;
                        //ue.nState = decode_modifier(nsevent);
                        break;

                    case NSEventTypeLeftMouseUp:
                    case NSEventTypeRightMouseUp:
                    case NSEventTypeOtherMouseUp:
                        ue.nType = UIE_MOUSE_UP;
                        ue.nCode = decode_mcb(nsevent); //decode_mcb(nsevent);
                        ue.nState = decode_modifier(nsevent);
                        ue.nState = lastMouseButton;
                        lastMouseButton = decode_modifier(nsevent);
                        pDragTarget = NULL;
                        break;

                    case NSEventTypeMouseMoved:
                    case NSEventTypeLeftMouseDragged:
                    case NSEventTypeRightMouseDragged:
                    case NSEventTypeOtherMouseDragged:
                        ue.nType = UIE_MOUSE_MOVE;
                        ue.nState = lastMouseButton;
                        //ue.nState = decode_modifier(nsevent);
                        break;

                    case NSEventTypeScrollWheel:
                        ue.nType = UIE_MOUSE_SCROLL;
                        ue.nCode = decode_mcd(nsevent);
                        ue.nState = decode_modifier(nsevent);
                        break;

                    case NSEventTypeKeyDown:
                        //TODO: implement mouse / keyboard button states
                        keyCode = [nsevent keyCode];
                        chars = [nsevent charactersIgnoringModifiers];
                        keysym = [chars characterAtIndex:0];

                        lsp_trace("Key Code: %hu", keyCode);
                        ue.nType = UIE_KEY_DOWN;
                        ue.nRawCode = keyCode;
                        ue.nCode = keysym;
                        ue.nState = decode_modifier(nsevent);
                        break;

                    case NSEventTypeKeyUp:
                        //TODO: implement mouse / keyboard button states
                        keyCode = [nsevent keyCode];
                        chars = [nsevent charactersIgnoringModifiers];
                        keysym = [chars characterAtIndex:0];

                        lsp_trace("Key Code: %hu", keyCode);
                        ue.nType = UIE_KEY_UP;
                        ue.nRawCode = keyCode;
                        ue.nCode = keysym;
                        ue.nState = decode_modifier(nsevent);
                        break;

                    case NSEventTypeMouseEntered:
                        ue.nType = UIE_MOUSE_IN;
                        break;

                    case NSEventTypeMouseExited:
                        ue.nType = UIE_MOUSE_OUT;
                        break;

                    case NSEventTypeFlagsChanged:
                        // Optional: Modifier key changes
                        break;

                    default:
                        return;  // Unhandled
                }

                // If your architecture supports redirection or grabs, simulate it here
                target->handle_event(&ue);
            }
        
            status_t CocoaDisplay::process_pending_events()
            {
                @autoreleasepool {
                    if (standaloneApp)
                    {
                        NSEvent *event;
                        while ((event = [NSApp  nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                inMode:NSDefaultRunLoopMode
                                                dequeue:YES]))
                        {
                            [NSApp sendEvent:event];
                            [NSApp updateWindows];
                        }
                    }
                }

                return STATUS_OK;
            }

            status_t CocoaDisplay::main_iteration()
            {
                return do_main_iteration(system::get_time_millis());
            }

            void CocoaDisplay::quit_main()
            {
                bExit = true;
            }
            
            status_t CocoaDisplay::wait_events(wssize_t millis)
            {
                if (millis <= 0)
                    return STATUS_OK;

                const int wtime = compute_poll_delay(system::get_time_millis() + millis, idle_interval());
                if (wtime <= 0)
                    return STATUS_OK;
                
                if (!standaloneApp) {
                    ipc::Thread::sleep(wtime);
                    return STATUS_OK;
                }
                
                // TODO: is there any reliable way to sleep until new message in the event queue occurs?
                @autoreleasepool {
                    const NSEvent * event = [NSApp  nextEventMatchingMask:NSEventMaskAny
                                                    untilDate:[NSDate distantPast]
                                                    inMode:NSDefaultRunLoopMode
                                                    dequeue:NO];
                    if (!event)
                        ipc::Thread::sleep(wtime);
                }
                
                return STATUS_OK;
            }

            IWindow *CocoaDisplay::create_window()
            {
                lsp_trace("create_window 1");
                CocoaWindow *wnd = new CocoaWindow(this, NULL, NULL, false);
                add_window(wnd);
                return wnd;
            }

            IWindow *CocoaDisplay::create_window(size_t screen)
            {
                lsp_trace("create_window 2");
                CocoaWindow *wnd = new CocoaWindow(this, NULL, NULL, false);
                add_window(wnd);
                return wnd;
            }

            IWindow *CocoaDisplay::create_window(void *handle)
            {
                lsp_trace("create_window 3");
                lsp_trace("handle = %p", handle);
                //CocoaWindow *wnd = new CocoaWindow(this, NULL, NULL, false);
                CocoaWindow *wnd = new CocoaWindow(this, (__bridge NSView*)handle, NULL, true);
                add_window(wnd);
                return wnd;
            }

            bool CocoaDisplay::add_window(CocoaWindow *wnd)
            {
                return vWindows.add(wnd);
            }

            bool CocoaDisplay::remove_window(CocoaWindow *wnd)
            {
                // Remove focus window
                //if (pFocusWindow == wnd)
                //    pFocusWindow = NULL;

                // Remove window from list
                if (!vWindows.premove(wnd))
                    return false;

                // Check if need to leave main cycle
                if (vWindows.size() <= 0)
                    bExit = true;
                return true;
            }

            CocoaWindow *CocoaDisplay::find_window(const nswindow_t & wnd)
            {
                const NSWindow * const nswnd = wnd.window;

                size_t n = vWindows.size();

                for (size_t i = 0; i < n; ++i)
                {
                    CocoaWindow *w = vWindows.uget(i);
                    if (w == NULL)
                        continue;
                    if (w->nswindow() == nswnd)
                        return w;
                }

                return NULL;
            }

            CocoaWindow *CocoaDisplay::find_topmost_grab_window()
            {
                // Highest priority group with at least one window wins.
                for (ssize_t g = __GRAB_TOTAL - 1; g >= 0; --g)
                {
                    lltl::parray<CocoaWindow> &arr = vGrab[g];
                    const size_t n = arr.size();
                    if (n == 0)
                        continue;
                    // Most recently added in this group is the topmost popup
                    // (matches widget framework expectations on Linux/Win).
                    return arr.uget(n - 1);
                }
                return NULL;
            }

            static bool point_inside_window(NSPoint screenPt, CocoaWindow *wnd)
            {
                if (wnd == NULL)
                    return false;
                NSWindow *nsWin = wnd->get_window_handler();
                if (nsWin == nil)
                    return false;
                NSRect f = [nsWin frame];
                return NSPointInRect(screenPt, f);
            }

            bool CocoaDisplay::dispatch_grabbed_event(void *eventPtr)
            {
                NSEvent *event = (NSEvent *) eventPtr;
                CocoaWindow *target = find_topmost_grab_window();
                if (target == NULL)
                    return false;

                // Compute screen coords of the click.
                NSPoint screenPt;
                if ([event window] != nil)
                    screenPt = [[event window] convertPointToScreen:[event locationInWindow]];
                else
                    screenPt = [event locationInWindow];

                // If the click landed inside any grabbing popup, do nothing
                // here — AppKit delivers the event to that popup's NSWindow
                // via the normal route.
                for (ssize_t g = __GRAB_TOTAL - 1; g >= 0; --g)
                {
                    lltl::parray<CocoaWindow> &arr = vGrab[g];
                    for (size_t i = 0, n = arr.size(); i < n; ++i)
                    {
                        if (point_inside_window(screenPt, arr.uget(i)))
                            return false;
                    }
                }

                // Click is outside every grabbing popup. Synthesize a
                // UIE_MOUSE_DOWN at popup-local coords (which will be outside
                // the popup's view bounds) and deliver it directly to the
                // topmost popup so the widget framework's outside-click logic
                // (e.g. Menu::hide()) fires.
                NSWindow *tgtWin = target->get_window_handler();
                NSRect tgtFrame = (tgtWin != nil) ? [tgtWin frame] : NSMakeRect(0,0,0,0);
                NSPoint local = NSMakePoint(screenPt.x - tgtFrame.origin.x,
                                            screenPt.y - tgtFrame.origin.y);

                event_t ue;
                init_event(&ue);
                ue.nLeft  = ssize_t(local.x);
                // Convert from Cocoa bottom-origin to top-origin.
                ue.nTop   = ssize_t(tgtFrame.size.height - local.y);
                ue.nTime  = timestamp_t([event timestamp] * 1000);

                NSEventType etype = [event type];
                switch (etype)
                {
                    case NSEventTypeLeftMouseDown:
                    case NSEventTypeRightMouseDown:
                    case NSEventTypeOtherMouseDown:
                        ue.nType = UIE_MOUSE_DOWN;
                        ue.nCode = decode_mcb(event);
                        break;
                    case NSEventTypeLeftMouseUp:
                    case NSEventTypeRightMouseUp:
                    case NSEventTypeOtherMouseUp:
                        ue.nType = UIE_MOUSE_UP;
                        ue.nCode = decode_mcb(event);
                        break;
                    default:
                        return false;
                }

                target->handle_event(&ue);
                // Do not consume — the host (DAW) chrome (e.g. window close
                // button) still needs to receive the original click.
                return false;
            }

            void CocoaDisplay::install_grab_monitor()
            {
                if (pGrabMonitor != NULL)
                    return;

                CocoaDisplay *self = this;
                NSEventMask mask = NSEventMaskLeftMouseDown
                                 | NSEventMaskRightMouseDown
                                 | NSEventMaskOtherMouseDown;
                id token = [NSEvent addLocalMonitorForEventsMatchingMask:mask
                                    handler:^NSEvent *(NSEvent *evt)
                                    {
                                        if (self->dispatch_grabbed_event(evt))
                                            return nil; // consumed
                                        return evt;
                                    }];
                pGrabMonitor = (void *) [token retain];
            }

            void CocoaDisplay::uninstall_grab_monitor()
            {
                if (pGrabMonitor == NULL)
                    return;
                id token = (id) pGrabMonitor;
                [NSEvent removeMonitor:token];
                [token release];
                pGrabMonitor = NULL;
            }

            status_t CocoaDisplay::grab_events(CocoaWindow *wnd, grab_t group)
            {
                if (wnd == NULL || group >= __GRAB_TOTAL)
                    return STATUS_BAD_ARGUMENTS;

                // Reject duplicate grab for the same window in any group.
                size_t total = 0;
                for (size_t i = 0; i < __GRAB_TOTAL; ++i)
                {
                    if (vGrab[i].index_of(wnd) >= 0)
                        return STATUS_DUPLICATED;
                    total += vGrab[i].size();
                }

                if (!vGrab[group].add(wnd))
                    return STATUS_NO_MEM;

                if (total == 0)
                    install_grab_monitor();
                return STATUS_OK;
            }

            status_t CocoaDisplay::ungrab_events(CocoaWindow *wnd)
            {
                bool found = false;
                size_t remaining = 0;
                for (size_t i = 0; i < __GRAB_TOTAL; ++i)
                {
                    if (vGrab[i].premove(wnd))
                        found = true;
                    remaining += vGrab[i].size();
                }
                if (!found)
                    return STATUS_NO_GRAB;
                if (remaining == 0)
                    uninstall_grab_monitor();
                return STATUS_OK;
            }

            bool CocoaDisplay::is_grabbing_events(const CocoaWindow *wnd) const
            {
                for (size_t i = 0; i < __GRAB_TOTAL; ++i)
                    if (vGrab[i].index_of(const_cast<CocoaWindow *>(wnd)) >= 0)
                        return true;
                return false;
            }


            void CocoaDisplay::destroy()
            {
                // Stop the display-wide iteration timer first so no tick can
                // fire into a half-destroyed display.
                if (pIterationTimer != NULL)
                {
                    [(NSTimer *) pIterationTimer invalidate];
                    pIterationTimer = NULL;
                }
                if (pIterationTimerProxy != NULL)
                {
                    LSPDisplayTimerProxy *proxy = (LSPDisplayTimerProxy *) pIterationTimerProxy;
                    [proxy invalidate];
                    [proxy release];
                    pIterationTimerProxy = NULL;
                }

                // Tear down any installed grab monitor so it cannot fire into
                // freed state if grabs were active at shutdown.
                uninstall_grab_monitor();
                for (size_t i = 0; i < __GRAB_TOTAL; ++i)
                    vGrab[i].clear();

                // Destroy font manager
            #ifdef USE_LIBFREETYPE
                sFontManager.destroy();
            #endif /* USE_LIBFREETYPE */

                if (standaloneApp)
                    [NSApp terminate:nil];

                IDisplay::destroy();
            }

            const MonitorInfo *CocoaDisplay::enum_monitors(size_t *count)
            {
                // Prepare result array
                lltl::darray<MonitorInfo> result;

                NSArray<NSScreen *> *screens = [NSScreen screens];
                NSUInteger nmonitors = [screens count];

                MonitorInfo *items = result.add_n(nmonitors);
                if (items == nullptr)
                    return nullptr;

                for (NSUInteger i = 0; i < nmonitors; ++i)
                {
                    MonitorInfo *di = &items[i];
                    new (&di->name, inplace_new_tag_t()) LSPString;

                    NSScreen *screen = screens[i];
                    NSRect frame = [screen frame];

                    // Set screen bounds
                    di->rect.nLeft   = static_cast<int>(frame.origin.x);
                    di->rect.nTop    = static_cast<int>(frame.origin.y);
                    di->rect.nWidth  = static_cast<int>(frame.size.width);
                    di->rect.nHeight = static_cast<int>(frame.size.height);

                    // Set primary flag (main screen)
                    di->primary = (screen == [NSScreen mainScreen]);

                    // No native way to get monitor name in Cocoa, so use fallback
                    LSPString monitorName;
                    monitorName.fmt_utf8("Monitor %lu", (unsigned long)i);
                    di->name.set_utf8(monitorName.get_utf8());
                }
                
                if (count)
                    *count = result.size();
                return result.release();
            }

            status_t CocoaDisplay::work_area_geometry(ws::rectangle_t *r)
            {
                if (r == nullptr)
                    return STATUS_BAD_ARGUMENTS;

                NSScreen *screen = [NSScreen mainScreen];
                if (!screen)
                    return STATUS_UNKNOWN_ERR;

                NSRect frame = [screen visibleFrame];  // Excludes Dock & menu bar

                r->nLeft   = static_cast<int>(frame.origin.x);
                r->nTop    = static_cast<int>(frame.origin.y);
                r->nWidth  = static_cast<int>(frame.size.width);
                r->nHeight = static_cast<int>(frame.size.height);

                return STATUS_OK;
            }

            status_t CocoaDisplay::screen_size(size_t screen, ssize_t *w, ssize_t *h)
            {
                NSScreen *mainScreen = [NSScreen mainScreen];
                if (mainScreen == nil)
                    return STATUS_UNKNOWN_ERR;

                NSRect frame = [mainScreen frame];
                CGFloat width = frame.size.width;
                CGFloat height = frame.size.height;

                if (width <= 0 || height <= 0)
                    return STATUS_UNKNOWN_ERR;

                if (w != NULL)
                    *w = static_cast<ssize_t>(width);
                if (h != NULL)
                    *h = static_cast<ssize_t>(height);

                return STATUS_OK;
            }


            status_t CocoaDisplay::add_font(const char *name, io::IInStream *is)
            {
                if ((name == NULL) || (is == NULL))
                    return STATUS_BAD_ARGUMENTS;

                status_t res    = STATUS_OK;

            #ifdef USE_LIBFREETYPE
                if ((res = sFontManager.add(name, is)) != STATUS_OK)
                    return res;
            #endif /* USE_LIBFREETYPE */
                
                return res;
            }

            status_t CocoaDisplay::add_font_alias(const char *name, const char *alias)
            {
                if ((name == NULL) || (alias == NULL))
                    return STATUS_BAD_ARGUMENTS;

                status_t res    = STATUS_OK;
            #ifdef USE_LIBFREETYPE
                if ((res = sFontManager.add_alias(name, alias)) != STATUS_OK)
                    return res;
            #endif /* USE_LIBFREETYPE */

                return res;
            }
            
            status_t CocoaDisplay::remove_font(const char *name)
            {
                if (name == NULL)
                    return STATUS_BAD_ARGUMENTS;

                status_t res = STATUS_OK;
            #ifdef USE_LIBFREETYPE
                if ((res = sFontManager.remove(name)) != STATUS_OK)
                    return res;
            #endif /* USE_LIBFREETYPE */

                return res;
            }

            void CocoaDisplay::remove_all_fonts()
            {
            #ifdef USE_LIBFREETYPE
                sFontManager.clear();
            #endif /* USE_LIBFREETYPE */
            }

            bool CocoaDisplay::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // Redirect the request to estimation surface
                pEstimation->begin();
                lsp_finally{ pEstimation->end(); };
                return pEstimation->get_font_parameters(f, fp);
            }

            bool CocoaDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                // Redirect the request to estimation surface
                pEstimation->begin();
                lsp_finally{ pEstimation->end(); };
                return pEstimation->get_text_parameters(f, tp, text);
            }

            bool CocoaDisplay::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                // Redirect the request to estimation surface
                pEstimation->begin();
                lsp_finally{ pEstimation->end(); };
                return pEstimation->get_text_parameters(f, tp, text, first, last);
            }

            status_t CocoaDisplay::get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top)
            {
                //TODO: can we detect the screen?
                ssize_t sw, sh;
                this->screen_size(0, &sw, &sh);

                NSPoint mouseLocation = [NSEvent mouseLocation];
                *screen = 0;
                *left = (size_t)mouseLocation.x;
                *top = sh - (size_t)mouseLocation.y;

                return STATUS_OK;
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_MACOSX */
