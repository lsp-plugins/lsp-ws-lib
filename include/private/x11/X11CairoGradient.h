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

#if defined(USE_LIBX11) && defined(USE_LIBCAIRO)

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ws/IGradient.h>

#include <cairo/cairo.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class LSP_HIDDEN_MODIFIER X11CairoGradient: public IGradient
            {
                protected:
                    cairo_pattern_t *pCP;

                public:
                    explicit X11CairoGradient(cairo_pattern_t *cp);
                    virtual ~X11CairoGradient();

                public:
                    void apply(cairo_t *cr);

                public:
                    virtual void add_color(float offset, float r, float g, float b, float a);
            };
        }
    }
} /* namespace lsp */

#endif /* defined(USE_LIBX11) && defined(USE_LIBCAIRO) */

#endif /* UI_X11_X11CAIROGRADIENT_H_ */
