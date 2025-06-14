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

#ifndef UI_COCOAWINDOW_H_
#define UI_COCOAWINDOW_H_

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_MACOSX

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/IWindow.h>

namespace lsp
{
    namespace ws
    {
        namespace cocoa
        {
            class CocoaWindow;

            class LSP_HIDDEN_MODIFIER CocoaDisplay: public IDisplay
            {
                private:
                    friend class CocoaWindow;

                public:
                    // Main loop management
                    virtual status_t            main() override;
                    virtual status_t            main_iteration() override;
                    virtual void                quit_main() override;

                    explicit CocoaDisplay();
                    virtual ~CocoaDisplay();

                    virtual status_t            init(int argc, const char **argv) override;
                    virtual void                destroy() override;

                    // Window management
                    virtual IWindow            *create_window() override;
                    virtual IWindow            *create_window(size_t screen) override;
                    virtual IWindow            *create_window(void *handle) override;

                    // Monitor management
                    virtual const MonitorInfo  *enum_monitors(size_t *count) override;
                    virtual status_t            work_area_geometry(ws::rectangle_t *r) override;

                    // Screen and monitor management
                    //virtual size_t              screens() override;
                    //virtual size_t              default_screen() override;
                    virtual status_t            screen_size(size_t screen, ssize_t *w, ssize_t *h) override;

                    virtual status_t            add_font(const char *name, io::IInStream *is) override;
                    virtual status_t            add_font_alias(const char *name, const char *alias) override;

                    bool                        add_window(CocoaWindow *wnd);
                    bool                        remove_window(CocoaWindow *wnd);

                protected:
                    volatile bool               bExit;                      // Indicator that forces to leave the main loop
                    volatile timestamp_t        nLastIdleCall;              // The time of last idle call
                    lltl::parray<CocoaWindow>   sTargets;                   // Targets for event delivery
                    lltl::parray<CocoaWindow>   vWindows;                   // All registered windows

                #ifdef USE_LIBFREETYPE
                    ft::FontManager             sFontManager;
                #endif /* USE_LIBFREETYPE */

                protected:
                    status_t                    do_main_iteration(timestamp_t ts);
                    void                        handle_event(void *event);
                    CocoaWindow                 *find_window(void *wnd);

            };
        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_MACOSX */

#endif /* COCOAWINDOW_H */