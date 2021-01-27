/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 19 дек. 2016 г.
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

#ifndef UI_X11_X11CAIROGRADIENT_H_
#define UI_X11_X11CAIROGRADIENT_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>

#ifdef USE_LIBCAIRO

#include <lsp-plug.in/ws/IGradient.h>
#include <cairo/cairo.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class X11CairoGradient: public IGradient
            {
                protected:
                    cairo_pattern_t *pCP;

                public:
                    explicit X11CairoGradient();
                    virtual ~X11CairoGradient();

                public:
                    void apply(cairo_t *cr);

                public:
                    virtual void add_color(float offset, float r, float g, float b, float a);
            };

            class X11CairoLinearGradient: public X11CairoGradient
            {
                public:
                    explicit inline X11CairoLinearGradient(float x0, float y0, float x1, float y1)
                    {
                        pCP = ::cairo_pattern_create_linear(x0, y0, x1, y1);
                    };

                    virtual ~X11CairoLinearGradient();
            };

            class X11CairoRadialGradient: public X11CairoGradient
            {
                public:
                    explicit inline X11CairoRadialGradient(float cx0, float cy0, float r0, float cx1, float cy1, float r1)
                    {
                        pCP = ::cairo_pattern_create_radial(cx0, cy0, r0, cx1, cy1, r1);
                    };

                    virtual ~X11CairoRadialGradient();
            };
        }
    }
} /* namespace lsp */

#endif /* USE_LIBCAIRO */

#endif /* UI_X11_X11CAIROGRADIENT_H_ */
