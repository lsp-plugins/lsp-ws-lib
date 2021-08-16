/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
        class IGradient
        {
            private:
                IGradient & operator = (const IGradient &);
                IGradient(const IGradient &);

            public:
                explicit IGradient();
                virtual ~IGradient();

            public:
                virtual void add_color(float offset, float r, float g, float b, float a=0.0f);

                void add_color(float offset, const Color &c);

                void add_color(float offset, const Color &c, float a);

                void add_color_rgb(float offset, uint32_t color);

                void add_color_rgba(float offset, uint32_t color);
        };
    }
}


#endif /* LSP_PLUG_IN_WS_IGRADIENT_H_ */
