/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 4 мая 2020 г.
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

#ifndef LSP_PLUG_IN_WS_IGRADIENT_H_
#define LSP_PLUG_IN_WS_IGRADIENT_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/runtime/Color.h>

namespace lsp
{
    namespace ws
    {
        /**
         * Gradient interface
         */
        class LSP_WS_LIB_PUBLIC IGradient
        {
            public:
                explicit IGradient();
                IGradient(const IGradient &) = delete;
                IGradient(IGradient &&) = delete;
                virtual ~IGradient();

                IGradient & operator = (const IGradient &) = delete;
                IGradient & operator = (IGradient &&) = delete;

            public:
                virtual void set_start(float r, float g, float b, float a=0.0f);
                virtual void set_start(const Color & c);
                virtual void set_start(const Color & c, float a);
                virtual void set_start(const Color *c);
                virtual void set_start(const Color *c, float a);
                virtual void set_start_rgb(uint32_t color);
                virtual void set_start_rgba(uint32_t color);

                virtual void set_stop(float r, float g, float b, float a=0.0f);
                virtual void set_stop(const Color & c);
                virtual void set_stop(const Color & c, float a);
                virtual void set_stop(const Color *c);
                virtual void set_stop(const Color *c, float a);
                virtual void set_stop_rgb(uint32_t color);
                virtual void set_stop_rgba(uint32_t color);
        };
    } /* namespace ws */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_WS_IGRADIENT_H_ */
