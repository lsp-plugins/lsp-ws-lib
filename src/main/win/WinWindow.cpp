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

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <private/win/WinDisplay.h>
#include <private/win/WinWindow.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            WinWindow::WinWindow(WinDisplay *core, IEventHandler *handler, bool wrapper):
                ws::IWindow(core, handler)
            {
            }

            WinWindow::~WinWindow()
            {
            }

            status_t WinWindow::init()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            void WinWindow::destroy()
            {
            }

            ISurface *WinWindow::get_surface()
            {
                return NULL;
            }

            void *WinWindow::handle()
            {
                return NULL;
            }

            ssize_t WinWindow::left()
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            ssize_t WinWindow::top()
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            ssize_t WinWindow::width()
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            ssize_t WinWindow::height()
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_left(ssize_t left)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_top(ssize_t top)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            ssize_t WinWindow::set_width(ssize_t width)
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            ssize_t WinWindow::set_height(ssize_t height)
            {
                return -STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::hide()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::show()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::show(IWindow *over)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            bool WinWindow::is_visible()
            {
                return false;
            }

            size_t WinWindow::screen()
            {
                return 0;
            }

            status_t WinWindow::set_caption(const char *ascii, const char *utf8)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_caption(char *text, size_t len)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::move(ssize_t left, ssize_t top)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::resize(ssize_t width, ssize_t height)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_geometry(const rectangle_t *realize)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_border_style(border_style_t style)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_border_style(border_style_t *style)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_geometry(rectangle_t *realize)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_absolute_geometry(rectangle_t *realize)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_size_constraints(const size_limit_t *c)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_size_constraints(size_limit_t *c)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::check_constraints()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::get_window_actions(size_t *actions)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_window_actions(size_t actions)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::handle_event(const event_t *ev)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::grab_events(grab_t group)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::ungrab_events()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_focus(bool focus)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::toggle_focus()
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_icon(const void *bgra, size_t width, size_t height)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_mouse_pointer(mouse_pointer_t ponter)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            mouse_pointer_t WinWindow::get_mouse_pointer()
            {
                return MP_NONE;
            }

            status_t WinWindow::set_class(const char *instance, const char *wclass)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            status_t WinWindow::set_role(const char *wrole)
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            bool WinWindow::has_parent() const
            {
                return false;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */


#endif /* PLATFORM_WINDOWS */
