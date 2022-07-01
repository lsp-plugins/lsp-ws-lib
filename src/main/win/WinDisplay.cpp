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

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <private/win/WinDisplay.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            WinDisplay::WinDisplay()
            {
            }

            WinDisplay::~WinDisplay()
            {
            }

            status_t WinDisplay::init(int argc, const char **argv)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            void WinDisplay::destroy()
            {
            }

            status_t WinDisplay::main()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::main_iteration()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            void WinDisplay::quit_main()
            {
            }

            status_t WinDisplay::wait_events(wssize_t millis)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            IWindow *WinDisplay::create_window()
            {
                return NULL;
            }

            IWindow *WinDisplay::create_window(size_t screen)
            {
                return NULL;
            }

            IWindow *WinDisplay::create_window(void *handle)
            {
                return NULL;
            }

            IWindow *WinDisplay::wrap_window(void *handle)
            {
                return NULL;
            }

            ISurface *WinDisplay::create_surface(size_t width, size_t height)
            {
                return NULL;
            }

            size_t WinDisplay::screens()
            {
                return 1;
            }

            size_t WinDisplay::default_screen()
            {
                return 0;
            }

            status_t WinDisplay::screen_size(size_t screen, ssize_t *w, ssize_t *h)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            const MonitorInfo *WinDisplay::enum_monitors(size_t *count)
            {
                return NULL;
            }

            // Clipboard management
            status_t WinDisplay::set_clipboard(size_t id, IDataSource *ds)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::get_clipboard(size_t id, IDataSink *dst)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            // Drag & Drop management
            const char * const *WinDisplay::get_drag_ctypes()
            {
                return NULL;
            }

            status_t WinDisplay::reject_drag()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::accept_drag(IDataSink *sink, drag_t action, bool internal, const rectangle_t *r)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            // Mouse pointer management
            status_t WinDisplay::get_pointer_location(size_t *screen, ssize_t *left, ssize_t *top)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            // Font management
            status_t WinDisplay::add_font(const char *name, const char *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, const io::Path *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, const LSPString *path)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font(const char *name, io::IInStream *is)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::add_font_alias(const char *name, const char *alias)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinDisplay::remove_font(const char *name)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            void WinDisplay::remove_all_fonts()
            {
                // TODO
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


