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

#include <lsp-plug.in/ws/IGradient.h>

namespace lsp
{
    namespace ws
    {
        static constexpr float k_color = 1.0f / 255.0f;

        IGradient::IGradient()
        {
        }

        IGradient::~IGradient()
        {
        }

        void IGradient::set_start(float r, float g, float b, float a)
        {
        }

        void IGradient::set_start(const Color &c)
        {
            set_start(c.red(), c.green(), c.blue(), c.alpha());
        }

        void IGradient::set_start(const Color &c, float a)
        {
            set_start(c.red(), c.green(), c.blue(), a);
        }

        void IGradient::set_start_rgb(uint32_t color)
        {
            set_start(
                (color & 0xff) * k_color,
                ((color >> 8) & 0xff) * k_color,
                ((color >> 16) & 0xff) * k_color,
                0.0f);
        }

        void IGradient::set_start_rgba(uint32_t color)
        {
            set_start(
                (color & 0xff) * k_color,
                ((color >> 8) & 0xff) * k_color,
                ((color >> 16) & 0xff) * k_color,
                ((color >> 24) & 0xff) * k_color);
        }

        void IGradient::set_stop(float r, float g, float b, float a)
        {
        }

        void IGradient::set_stop(const Color &c)
        {
            set_stop(c.red(), c.green(), c.blue(), c.alpha());
        }

        void IGradient::set_stop(const Color &c, float a)
        {
            set_stop(c.red(), c.green(), c.blue(), a);
        }

        void IGradient::set_stop_rgb(uint32_t color)
        {
            set_stop(
                ((color >> 16) & 0xff) * k_color,
                ((color >> 8) & 0xff) * k_color,
                (color & 0xff) * k_color,
                0.0f);
        }

        void IGradient::set_stop_rgba(uint32_t color)
        {
            set_stop(
                ((color >> 16) & 0xff) * k_color,
                ((color >> 8) & 0xff) * k_color,
                (color & 0xff) * k_color,
                ((color >> 24) & 0xff) * k_color);
        }

    } /* namespace ws */
} /* namespace lsp */

