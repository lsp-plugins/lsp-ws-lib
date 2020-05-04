/*
 * IGradient.h
 *
 *  Created on: 4 мая 2020 г.
 *      Author: sadko
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
