/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *           (C) 2025 Marvin Edeler <marvin.edeler@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 12 June 2025
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

#ifdef PLATFORM_MACOSX

#include <lsp-plug.in/common/types.h>

#include <private/cocoa/CocoaCairoGradient.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            CocoaCairoGradient::CocoaCairoGradient(const linear_t & params)
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

            CocoaCairoGradient::CocoaCairoGradient(const radial_t & params)
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

            CocoaCairoGradient::~CocoaCairoGradient()
            {
                drop_pattern();
            }

            void CocoaCairoGradient::drop_pattern()
            {
                if (pCP != NULL)
                {
                    cairo_pattern_destroy(pCP);
                    pCP = NULL;
                }
            }

            void CocoaCairoGradient::apply(cairo_t *cr)
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

            void CocoaCairoGradient::set_start(float r, float g, float b, float a)
            {
                drop_pattern();

                sStart.r = r;
                sStart.g = g;
                sStart.b = b;
                sStart.a = 1.0f - a;
            }

            void CocoaCairoGradient::set_stop(float r, float g, float b, float a)
            {
                drop_pattern();

                sEnd.r = r;
                sEnd.g = g;
                sEnd.b = b;
                sEnd.a = 1.0f - a;
            }

        } /* namespace cocoa */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(PLATFORM_MACOSX) */
