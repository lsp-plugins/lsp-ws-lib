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

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/ws/IDisplay.h>

#include <windows.h>
#include <wincodec.h>
#include <d2d1.h>
#include <dwrite.h>

//-----------------------------------------------------------------------------
// Some specific definitions
#ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL  0x020e
#endif /* WM_MOUSEHWHEEL */

#ifndef VK_IME_ON
    #define VK_IME_ON       0x16
#endif /* VK_IME_ON */

#ifndef VK_IME_OFF
    #define VK_IME_OFF      0x1a
#endif /* VK_IME_ON */

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinWindow;
            class WinFontFileLoader;

            class WinDisplay: public IDisplay
            {
                private:
                    friend class WinWindow;
                    friend class WinDDSurface;

                public:
                    static const WCHAR         *WINDOW_CLASS_NAME;

                protected:
                    typedef struct font_t
                    {
                        char                   *name;           // Name of the font
                        union
                        {
                            char                   *alias;          // Font alias (the symbolic name of the font)
                            WCHAR                  *wname;          // The real font name
                        };
                        IDWriteFontFileLoader       *file;          // Font file loader
                        IDWriteFontCollectionLoader *loader;        // Font collection loader
                        IDWriteFontFamily           *family;        // Font family
                        IDWriteFontCollection       *collection;    // Font collection
                    } font_t;

                    typedef lltl::pphash<LSPString, IDWriteFontFamily> font_cache_t;
                    typedef lltl::pphash<char, font_t> custom_font_cache_t;

                protected:
                    static volatile atomic_t    hLock;
                    static HHOOK                hMouseHook;
                    static HHOOK                hKeyboardHook;
                    static WinDisplay          *pHandlers;

                protected:
                    volatile bool               bExit;                      // Indicator that forces to leave the main loop
                    ID2D1Factory               *pD2D1Factroy;               // Direct2D factory
                    IWICImagingFactory         *pWICFactory;                // WIC Imaging Factory
                    IDWriteFactory             *pDWriteFactory;             // DirectWrite Factory for text
                    ATOM                        hWindowClass;               // Window class
                    LSPString                   sDflFontFamily;             // Default font family name
                    MSG                         sPendingMessage;            // Currently pending message
                    HCURSOR                     vCursors[__MP_COUNT];       // Cursor handles (cached)
                    lltl::darray<MonitorInfo>   vMonitors;                  // Monitor information
                    font_cache_t                vFontCache;                 // Font cache
                    custom_font_cache_t         vCustomFonts;               // Custom fonts
                    WinDisplay                 *pNextHandler;               // Next hook handler in the chain of handlers
                    lltl::parray<WinWindow>     vGrab[__GRAB_TOTAL];        // Grab queue according to the priority
                    lltl::parray<WinWindow>     sTargets;                   // Targets for event delivery

                protected:
                    void                        do_destroy();
                    status_t                    do_main_iteration(timestamp_t ts);
                    static void                 drop_monitors(lltl::darray<MonitorInfo> *list);
                    IDWriteTextLayout          *create_text_layout(const Font &f, const WCHAR *fname, IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *string, size_t length);
                    IDWriteFontFamily          *get_font_family(const Font &f, LSPString *name, font_t **custom);
                    bool                        get_font_metrics(const Font &f, IDWriteFontFamily *ff, DWRITE_FONT_METRICS *metrics);
                    static void                 drop_font_cache(font_cache_t *cache);
                    bool                        create_font_cache();
                    void                        drop_font(font_t *f);
                    static font_t              *alloc_font(const char *name);
                    font_t                     *get_custom_font_collection(const char *name);
                    bool                        try_get_text_parameters(const Font &f, const WCHAR *fname, IDWriteFontCollection *fc, IDWriteFontFamily *ff, text_parameters_t *tp, const WCHAR *text, ssize_t length);
                    status_t                    install_windows_hooks();
                    status_t                    uninstall_windows_hooks();
                    void                        process_mouse_hook(int nCode, WPARAM wParam, LPARAM lParam);
                    void                        process_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam);
                    bool                        fill_targets();

                protected:
                    static LRESULT CALLBACK     window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
                    static WINBOOL CALLBACK     enum_monitor_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM dwParam);
                    static LRESULT CALLBACK     mouse_hook(int nCode, WPARAM wParam, LPARAM lParam);
                    static LRESULT CALLBACK     keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam);

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
                    virtual status_t            add_font(const char *name, io::IInStream *is) override;
                    virtual status_t            add_font_alias(const char *name, const char *alias) override;
                    virtual status_t            remove_font(const char *name) override;
                    virtual void                remove_all_fonts() override;

                    // Font and text parameter estimation
                    virtual bool                get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool                get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;

                public:
                    HCURSOR                     translate_cursor(mouse_pointer_t cursor);
                    inline ID2D1Factory        *d2d_factory()           { return pD2D1Factroy;      }
                    inline IWICImagingFactory  *wic_factory()           { return pWICFactory;       }

                    status_t                    grab_events(WinWindow *wnd, grab_t group);
                    status_t                    ungrab_events(WinWindow *wnd);
            };
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINDISPLAY_H_ */
