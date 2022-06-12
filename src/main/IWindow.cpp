/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 дек. 2016 г.
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

#include <lsp-plug.in/ws/IWindow.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ws
    {
        IWindow::IWindow(IDisplay *dpy, IEventHandler *handler)
        {
            pDisplay    = dpy;
            pHandler    = handler;
        }

        IWindow::~IWindow()
        {
            pHandler    = NULL;
        }

        status_t IWindow::init()
        {
            return STATUS_OK;
        }

        void IWindow::destroy()
        {
            pDisplay    = NULL;
            pHandler    = NULL;
        }

        ISurface *IWindow::get_surface()
        {
            return NULL;
        }

        ssize_t IWindow::left()
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            return (result == STATUS_OK) ? r.nLeft : -1;
        }

        ssize_t IWindow::top()
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            return (result == STATUS_OK) ? r.nTop : -1;
        }

        ssize_t IWindow::width()
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            return (result == STATUS_OK) ? r.nWidth : -1;
        }

        ssize_t IWindow::height()
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            return (result == STATUS_OK) ? r.nHeight : -1;
        }

        bool IWindow::is_visible()
        {
            return false;
        }

        size_t IWindow::screen()
        {
            return 0;
        }

        void *IWindow::handle()
        {
            return NULL;
        }

        status_t IWindow::move(ssize_t left, ssize_t top)
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
                return result;

            r.nLeft         = left;
            r.nTop          = top;
            return set_geometry(&r);
        }

        status_t IWindow::resize(ssize_t width, ssize_t height)
        {
//            lsp_trace("w=%d, h=%d", int(width), int(height));

            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
            {
//                lsp_trace("get_geometry falied: err=%d", int(result));
                return result;
            }

            r.nWidth        = width;
            r.nHeight       = height;
            return set_geometry(&r);
        }

        status_t IWindow::set_geometry(ssize_t left, ssize_t top, ssize_t width, ssize_t height)
        {
            rectangle_t r;

            r.nLeft     = left;
            r.nTop      = top;
            r.nWidth    = width;
            r.nHeight   = height;

            return set_geometry(&r);
        }

        status_t IWindow::set_geometry(const rectangle_t *realize)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::set_border_style(border_style_t style)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::get_border_style(border_style_t *style)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::get_geometry(rectangle_t *realize)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::get_absolute_geometry(rectangle_t *realize)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::hide()
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::show()
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::show(IWindow *over)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::set_left(ssize_t left)
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
                return result;

            r.nLeft         = left;
            return set_geometry(&r);
        }

        status_t IWindow::set_top(ssize_t top)
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
                return result;

            r.nTop          = top;
            return set_geometry(&r);
        }

        ssize_t IWindow::set_width(ssize_t width)
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
                return result;

            r.nWidth        = width;
            return set_geometry(&r);
        }

        ssize_t IWindow::set_height(ssize_t height)
        {
            rectangle_t r;
            status_t result = get_geometry(&r);
            if (result != STATUS_OK)
                return result;

            r.nHeight       = height;
            return set_geometry(&r);
        }

        status_t IWindow::set_visibility(bool visible)
        {
            return (visible) ? show() : hide();
        }

        status_t IWindow::set_size_constraints(const size_limit_t *c)
        {
            return STATUS_OK;
        }

        status_t IWindow::set_size_constraints(ssize_t min_width, ssize_t min_height, ssize_t max_width, ssize_t max_height)
        {
            size_limit_t sr;
            sr.nMinWidth        = min_width;
            sr.nMinHeight       = min_height;
            sr.nMaxWidth        = max_width;
            sr.nMaxHeight       = max_height;

            return set_size_constraints(&sr);
        }

        status_t IWindow::check_constraints()
        {
            return STATUS_OK;
        }

        status_t IWindow::get_size_constraints(size_limit_t *c)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::set_min_width(ssize_t value)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMinWidth    = value;
            return set_size_constraints(&sr);
        }

        status_t IWindow::set_min_height(ssize_t value)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMinHeight   = value;
            return set_size_constraints(&sr);
        }

        status_t IWindow::set_max_width(ssize_t value)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMaxWidth    = value;
            return set_size_constraints(&sr);
        }

        status_t IWindow::set_max_height(ssize_t value)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMaxHeight   = value;
            return set_size_constraints(&sr);
        }

        status_t IWindow::set_min_size(ssize_t width, ssize_t height)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMinWidth    = width;
            sr.nMinHeight   = height;
            return set_size_constraints(&sr);
        }

        status_t IWindow::set_max_size(ssize_t width, ssize_t height)
        {
            size_limit_t sr;
            status_t result = get_size_constraints(&sr);
            if (result != STATUS_OK)
                return result;
            sr.nMaxWidth    = width;
            sr.nMaxHeight   = height;
            return set_size_constraints(&sr);
        }


        status_t IWindow::set_focus(bool focus)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::toggle_focus()
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::set_caption(const char *ascii, const char *utf8)
        {
            return STATUS_OK;
        }

        status_t IWindow::get_caption(char *text, size_t len)
        {
            if (len < 1)
                return STATUS_TOO_BIG;
            text[0] = '\0';
            return STATUS_OK;
        }

        status_t IWindow::set_icon(const void *bgra, size_t width, size_t height)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::get_window_actions(size_t *actions)
        {
            if (actions != NULL)
                *actions = 0;
            return STATUS_OK;
        }

        status_t IWindow::set_window_actions(size_t actions)
        {
            return STATUS_OK;
        }

        status_t IWindow::set_mouse_pointer(mouse_pointer_t pointer)
        {
            return STATUS_OK;
        }

        mouse_pointer_t IWindow::get_mouse_pointer()
        {
            return MP_DEFAULT;
        }

        status_t IWindow::grab_events(grab_t grab)
        {
            lsp_error("not implemented");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWindow::ungrab_events()
        {
            return STATUS_NO_GRAB;
        }

        status_t IWindow::set_class(const char *instance, const char *wclass)
        {
            return STATUS_OK;
        }

        status_t IWindow::set_role(const char *wrole)
        {
            return STATUS_OK;
        }

        bool IWindow::has_parent() const
        {
            return false;
        }
    } /* namespace ws */
} /* namespace lsp */
