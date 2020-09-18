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

#include <lsp-plug.in/ws/IGradient.h>

namespace lsp
{
    namespace ws
    {
        IGradient::IGradient()
        {
        }

        IGradient::~IGradient()
        {
        }

        void IGradient::add_color(float offset, float r, float g, float b, float a)
        {
        }

        void IGradient::add_color(float offset, const Color &c)
        {
            add_color(offset, c.red(), c.green(), c.blue(), c.alpha());
        }

        void IGradient::add_color(float offset, const Color &c, float a)
        {
            add_color(offset, c.red(), c.green(), c.blue(), a);
        }

        void IGradient::add_color_rgb(float offset, uint32_t color)
        {
            add_color(offset,
                (color & 0xff) / 255.0f,
                ((color >> 8) & 0xff) / 255.0f,
                ((color >> 16) & 0xff) / 255.0f,
                0.0f
            );
        }

        void IGradient::add_color_rgba(float offset, uint32_t color)
        {
            add_color(offset,
                (color & 0xff) / 255.0f,
                ((color >> 8) & 0xff) / 255.0f,
                ((color >> 16) & 0xff) / 255.0f,
                ((color >> 24) & 0xff) / 255.0f
            );
        }
    }
}

