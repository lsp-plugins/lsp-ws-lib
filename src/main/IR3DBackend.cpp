/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 24 апр. 2019 г.
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

#include <lsp-plug.in/ws/IR3DBackend.h>

namespace lsp
{
    namespace ws
    {
        IR3DBackend::IR3DBackend(IDisplay *dpy, r3d::backend_t *backend, void *parent, void *window)
        {
            pBackend    = backend;
            hParent     = parent;
            hWindow     = window;
            pDisplay    = dpy;
        }
        
        IR3DBackend::~IR3DBackend()
        {
            pBackend    = NULL;
            hParent     = NULL;
            hWindow     = NULL;
        }

        status_t IR3DBackend::destroy()
        {
            if (pBackend != NULL)
            {
                pBackend->destroy(pBackend);
                pDisplay->deregister_backend(this);
            }

            pBackend    = NULL;
            hWindow     = NULL;
            hParent     = NULL;
            pDisplay    = NULL;

            return STATUS_OK;
        }

        void IR3DBackend::replace_backend(r3d::backend_t *backend, void *window)
        {
            if (pBackend != NULL)
            {
                // Replace matrices
                r3d::mat4_t tmp;
                if (pBackend->get_matrix(pBackend, r3d::MATRIX_PROJECTION, &tmp) == STATUS_OK)
                    backend->set_matrix(backend, r3d::MATRIX_PROJECTION, &tmp);
                if (pBackend->get_matrix(pBackend, r3d::MATRIX_VIEW, &tmp) == STATUS_OK)
                    backend->set_matrix(backend, r3d::MATRIX_VIEW, &tmp);
                if (pBackend->get_matrix(pBackend, r3d::MATRIX_WORLD, &tmp) == STATUS_OK)
                    backend->set_matrix(backend, r3d::MATRIX_WORLD, &tmp);

                // Copy location
                ssize_t l, t, w, h;
                if (pBackend->get_location(pBackend, &l, &t, &w, &h) == STATUS_OK)
                    backend->locate(backend, l, t, w, h);

                // Copy default color
                r3d::color_t c;
                if (pBackend->get_bg_color(pBackend, &c) == STATUS_OK)
                    backend->set_bg_color(backend, &c);

                // Destroy previous backend
                pBackend->destroy(pBackend);
            }

            pBackend    = backend;
            hWindow     = window;
        }

        status_t IR3DBackend::sync()
        {
            return (pBackend != NULL) ? pBackend->sync(pBackend) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::locate(ssize_t left, ssize_t top, ssize_t width, ssize_t height)
        {
            return (pBackend != NULL) ? pBackend->locate(pBackend, left, top, width, height) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::get_location(ssize_t *left, ssize_t *top, ssize_t *width, ssize_t *height)
        {
            return (pBackend != NULL) ? pBackend->get_location(pBackend, left, top, width, height) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::begin_draw()
        {
            return (pBackend != NULL) ? pBackend->start(pBackend) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::end_draw()
        {
            return (pBackend != NULL) ? pBackend->finish(pBackend) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::read_pixels(void *buf, r3d::pixel_format_t format)
        {
            return (pBackend != NULL) ? pBackend->read_pixels(pBackend, buf, format) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::set_matrix(r3d::matrix_type_t type, const r3d::mat4_t *m)
        {
            return (pBackend != NULL) ? pBackend->set_matrix(pBackend, type, m) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::get_matrix(r3d::matrix_type_t type, r3d::mat4_t *m)
        {
            return (pBackend != NULL) ? pBackend->get_matrix(pBackend, type, m) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::set_lights(const r3d::light_t *lights, size_t count)
        {
            return (pBackend != NULL) ? pBackend->set_lights(pBackend, lights, count) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::draw_primitives(const r3d::buffer_t *buffer)
        {
            return (pBackend != NULL) ? pBackend->draw_primitives(pBackend, buffer) : STATUS_BAD_STATE;
        }

        status_t IR3DBackend::set_bg_color(const r3d::color_t *color)
        {
            return (pBackend != NULL) ? pBackend->set_bg_color(pBackend, color) : STATUS_BAD_STATE;
        }

    } /* namespace io */
} /* namespace lsp */
