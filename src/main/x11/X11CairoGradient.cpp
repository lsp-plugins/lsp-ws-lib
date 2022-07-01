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

#if defined(USE_LIBX11) && defined(USE_LIBCAIRO)

#include <lsp-plug.in/common/types.h>

#include <private/x11/X11CairoGradient.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            X11CairoGradient::X11CairoGradient()
            {
                pCP = NULL;
            }

            X11CairoGradient::~X11CairoGradient()
            {
                if (pCP != NULL)
                {
                    cairo_pattern_destroy(pCP);
                    pCP = NULL;
                }
            }

            void X11CairoGradient::add_color(float offset, float r, float g, float b, float a)
            {
                if (pCP == NULL)
                    return;

                cairo_pattern_add_color_stop_rgba(pCP, offset, r, g, b, 1.0f - a);
            }

            void X11CairoGradient::apply(cairo_t *cr)
            {
                if (pCP == NULL)
                    return;
                cairo_set_source(cr, pCP);
            }

            X11CairoLinearGradient::~X11CairoLinearGradient()
            {
            }

            X11CairoRadialGradient::~X11CairoRadialGradient()
            {
            }
        }
    }
} /* namespace lsp */

#endif /* defined(USE_LIBX11) && defined(USE_LIBCAIRO) */
