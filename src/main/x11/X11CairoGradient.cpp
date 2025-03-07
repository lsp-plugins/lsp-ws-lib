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
            X11CairoGradient::X11CairoGradient(const linear_t & params)
            {
                pCP         = NULL;

                sLinear     = params;

                sStart.r    = 0.0f;
                sStart.g    = 0.0f;
                sStart.b    = 0.0f;
                sStart.a    = 1.0f;

                sEnd.r      = 1.0f;
                sEnd.g      = 1.0f;
                sEnd.b      = 1.0f;
                sEnd.a      = 0.0f;

                bLinear     = true;
            }

            X11CairoGradient::X11CairoGradient(const radial_t & params)
            {
                pCP         = NULL;

                sRadial     = params;

                sStart.r    = 0.0f;
                sStart.g    = 0.0f;
                sStart.b    = 0.0f;
                sStart.a    = 1.0f;

                sEnd.r      = 1.0f;
                sEnd.g      = 1.0f;
                sEnd.b      = 1.0f;
                sEnd.a      = 0.0f;

                bLinear     = false;
            }

            X11CairoGradient::~X11CairoGradient()
            {
                drop_pattern();
            }

            void X11CairoGradient::drop_pattern()
            {
                if (pCP != NULL)
                {
                    cairo_pattern_destroy(pCP);
                    pCP = NULL;
                }
            }

            void X11CairoGradient::apply(cairo_t *cr)
            {
                if (pCP == NULL)
                {
                    pCP = (bLinear) ?
                        ::cairo_pattern_create_linear(sLinear.x1, sLinear.y1, sLinear.x2, sLinear.y2) :
                        ::cairo_pattern_create_radial(sRadial.x1, sRadial.y1, 0, sRadial.x2, sRadial.y2, sRadial.r);

                    ::cairo_pattern_add_color_stop_rgba(pCP, 0.0f, sStart.r, sStart.g, sStart.b, sStart.a);
                    ::cairo_pattern_add_color_stop_rgba(pCP, 1.0f, sEnd.r, sEnd.g, sEnd.b, sEnd.a);
                }

                cairo_set_source(cr, pCP);
            }

            void X11CairoGradient::set_start(float r, float g, float b, float a)
            {
                drop_pattern();

                sStart.r = r;
                sStart.g = g;
                sStart.b = b;
                sStart.a = 1.0f - a;
            }

            void X11CairoGradient::set_stop(float r, float g, float b, float a)
            {
                drop_pattern();

                sEnd.r = r;
                sEnd.g = g;
                sEnd.b = b;
                sEnd.a = 1.0f - a;
            }

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(USE_LIBX11) && defined(USE_LIBCAIRO) */
