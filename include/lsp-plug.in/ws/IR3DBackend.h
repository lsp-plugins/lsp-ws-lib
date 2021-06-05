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

#ifndef LSP_PLUG_IN_WS_IR3DBACKEND_H_
#define LSP_PLUG_IN_WS_IR3DBACKEND_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/r3d/iface/backend.h>
#include <lsp-plug.in/ws/IDisplay.h>

namespace lsp
{
    namespace ws
    {
        /**
         * This class is a wrapper around r3d_backend_t that allows dynamic change of backend
         */
        class IR3DBackend
        {
            private:
                IR3DBackend & operator = (const IR3DBackend &);
                IR3DBackend (const IR3DBackend &);

                friend class        IDisplay;

            protected:
                r3d::backend_t     *pBackend;   // Currently used backend
                void               *hParent;    // Parent window
                void               *hWindow;    // Currently used window
                IDisplay           *pDisplay;

            protected:
                explicit        IR3DBackend(IDisplay *dpy, r3d::backend_t *backend, void *parent, void *window);
                void            replace_backend(r3d::backend_t *factory, void *window);

            public:
                ~IR3DBackend();

            public:
                status_t        destroy();

                /**
                 * Return native window handle if it is present
                 */
                inline void    *handle()       { return hWindow; };

                inline bool     valid() const   { return pBackend != NULL; }

                status_t        locate(ssize_t left, ssize_t top, ssize_t width, ssize_t height);
                status_t        get_location(ssize_t *left, ssize_t *top, ssize_t *width, ssize_t *height);

                status_t        begin_draw();
                status_t        sync();
                status_t        read_pixels(void *buf, size_t stride, r3d::pixel_format_t format);
                status_t        end_draw();

                status_t        set_matrix(r3d::matrix_type_t type, const r3d::mat4_t *m);
                status_t        get_matrix(r3d::matrix_type_t type, r3d::mat4_t *m);
                status_t        set_lights(const r3d::light_t *lights, size_t count);
                status_t        draw_primitives(const r3d::buffer_t *buffer);
                status_t        set_bg_color(const r3d::color_t *color);
        };
    
    } /* namespace io */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_WS_IR3DBACKEND_H_ */
