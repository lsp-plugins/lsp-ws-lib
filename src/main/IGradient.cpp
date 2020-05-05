/*
 * IGradient.cpp
 *
 *  Created on: 4 мая 2020 г.
 *      Author: sadko
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

