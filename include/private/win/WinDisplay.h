/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef PRIVATE_WIN_WINDISPLAY_H_
#define PRIVATE_WIN_WINDISPLAY_H_

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/ws/IDisplay.h>

#include <windows.h>
#include <d2d1.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinWindow;

            class WinDisplay: public IDisplay
            {
                private:
                    friend class WinWindow;

                public:
                    static const WCHAR         *WINDOW_CLASS_NAME;

                protected:
                    volatile bool               bExit;                  // Indicator that forces to leave the main loop
                    ID2D1Factory               *pD2D1Factroy;           // Direct2D factory
                    MSG                         sPendingMessage;        // Currently pending message
                    HCURSOR                     vCursors[__MP_COUNT];   // Cursor handles (cached)
                    lltl::darray<MonitorInfo>   vMonitors;              // Monitor information

                protected:
                    status_t                    do_main_iteration(timestamp_t ts);
                    static void                 drop_monitors(lltl::darray<MonitorInfo> *list);

                protected:
                    static LRESULT CALLBACK     window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
                    static WINBOOL CALLBACK     enum_monitor_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM dwParam);

                public:
                    explicit WinDisplay();
                    virtual ~WinDisplay();

                    virtual status_t            init(int argc, const char **argv) override;
                    virtual void                destroy() override;

                public:
                    // Main loop management
                    virtual status_t            main() override;
                    virtual status_t            main_iteration() override;
                    virtual void                quit_main() override;
                    virtual status_t            wait_events(wssize_t millis) override;

                    // Window management
                    virtual IWindow            *create_window() override;
                    virtual IWindow            *create_window(size_t screen) override;
                    virtual IWindow            *create_window(void *handle) override;
                    virtual IWindow            *wrap_window(void *handle) override;
                    virtual ISurface           *create_surface(size_t width, size_t height) override;

                    // Screen and monitor management
                    virtual size_t              screens() override;
                    virtual size_t              default_screen() override;
                    virtual status_t            screen_size(size_t screen, ssize_t *w, ssize_t *h) override;
                    virtual const MonitorInfo  *enum_monitors(size_t *count) override;

                    // Clipboard management
                    virtual status_t            set_clipboard(size_t id, IDataSource *ds) override;
                    virtual status_t            get_clipboard(size_t id, IDataSink *dst) override;

                    // Drag & Drop management
                    virtual const char * const *get_drag_ctypes() override;
                    virtual status_t            reject_drag() override;
                    virtual status_t            accept_drag(IDataSink *sink, drag_t action, bool internal, const rectangle_t *r) override;

                    // Mouse pointer management
                    virtual status_t            get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top) override;

                    // Font management
                    virtual status_t            add_font(const char *name, const char *path) override;
                    virtual status_t            add_font(const char *name, const io::Path *path) override;
                    virtual status_t            add_font(const char *name, const LSPString *path) override;
                    virtual status_t            add_font(const char *name, io::IInStream *is) override;
                    virtual status_t            add_font_alias(const char *name, const char *alias) override;
                    virtual status_t            remove_font(const char *name) override;
                    virtual void                remove_all_fonts() override;

                public:
                    HCURSOR                     translate_cursor(mouse_pointer_t cursor);
                    inline ID2D1Factory        *d2d_factory()           { return pD2D1Factroy;     }
            };
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINDISPLAY_H_ */
