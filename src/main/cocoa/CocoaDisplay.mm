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
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/ws/types.h>
#include <lsp-plug.in/ws/keycodes.h>
#include <lsp-plug.in/ws/cocoa/decode.h>

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

#include <private/cocoa/CocoaDisplay.h>
#include <private/cocoa/CocoaWindow.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            CocoaDisplay::CocoaDisplay(): IDisplay()
            {
               bExit                   = false;
            }

            CocoaDisplay::~CocoaDisplay()
            {
            }

            status_t CocoaDisplay::init(int argc, const char **argv)
            {
                if (NSApp == NULL) {
                    [NSApplication sharedApplication];
                    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                    [NSApp activateIgnoringOtherApps:YES];
                    standaloneApp = true;
                } else 
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

                return IDisplay::init(argc, argv);
            }

            status_t CocoaDisplay::main() {
                while (!bExit) {
                    timestamp_t ts = system::get_time_millis();

                    // Do one main iteration
                    status_t result = do_main_iteration(ts);
                    
                    if (result != STATUS_OK)
                            return result;
                }
                
                return STATUS_OK;
            }

            void CocoaDisplay::get_enviroment_frame_sizes() {
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

            size_t CocoaDisplay::get_window_title_height() {
                return titleHeight;
            }

            size_t CocoaDisplay::get_window_border_width() {
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

            status_t CocoaDisplay::do_main_iteration(timestamp_t ts) {
                // Here, any queued Cocoa events already handled via sendEvent.
                // Use this to do your own rendering / app logic.
                @autoreleasepool {
                    if (standaloneApp) {
                        NSEvent *event;
                        while (
                            (
                                event = [NSApp  nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                inMode:NSDefaultRunLoopMode
                                                dequeue:YES] 

                            )
                        )
                        {
                            [NSApp sendEvent:event];
                            [NSApp updateWindows];

                            //Dispatch to custom handler
                            //handle_event(event);
                        } 
                    }

                    // Handle internal tasks
                    status_t result = process_pending_tasks(ts);

                #ifdef USE_LIBFREETYPE
                    sFontManager.gc();
                #endif

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

            void CocoaDisplay::handle_event(void *event)
            {
                NSEvent *nsevent = (__bridge NSEvent *)event;

                if (!nsevent)
                    return;

                NSEventType type = [nsevent type];
                
                NSWindow *nsWindow = [nsevent window];
                CocoaWindow *target = find_window(nsWindow);

                if (!target)
                    return;
                
                event_t ue = {};
                init_event(&ue);
                ue.nTime = timestamp_t([nsevent timestamp] * 1000);

                unsigned short keyCode = 65535;
                NSString *chars = @"";
                unichar keysym = 0;

                NSPoint locInWindow = [nsevent locationInWindow];
                NSView *targetView = [[nsWindow contentView] hitTest:locInWindow];
                NSPoint locInView = [targetView convertPoint:locInWindow fromView:nil];
                NSRect cFrame = [targetView frame];

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



            status_t CocoaDisplay::main_iteration() {
                timestamp_t ts = system::get_time_millis();
                return do_main_iteration(ts);
            }

            void CocoaDisplay::quit_main() {
                bExit = true;
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

            CocoaWindow *CocoaDisplay::find_window(void *wnd) {
                NSWindow *nswnd = (__bridge NSWindow *)wnd;
                
                size_t n = vWindows.size();

                for (size_t i = 0; i < n; ++i) {
                    CocoaWindow *w = vWindows.uget(i);
                    if (w == NULL)
                        continue;
                    if (w->nswindow() == nswnd)
                        return w;
                }

                return NULL;
            }


            void CocoaDisplay::destroy()
            {
  
                // Destroy font manager
            #ifdef USE_LIBFREETYPE
                sFontManager.destroy();
            #endif /* USE_LIBFREETYPE */

                if (standaloneApp)
                    [NSApp terminate:nil];

                IDisplay::destroy();
            }

            const MonitorInfo *CocoaDisplay::enum_monitors(size_t *count) {
                // Prepare result array
                lltl::darray<MonitorInfo> result;

                NSArray<NSScreen *> *screens = [NSScreen screens];
                NSUInteger nmonitors = [screens count];

                MonitorInfo *items = result.add_n(nmonitors);
                if (items == nullptr)
                    return nullptr;

                for (NSUInteger i = 0; i < nmonitors; ++i) {
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

                status_t res;
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