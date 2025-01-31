/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 21 янв. 2025 г.
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

#include <lsp-plug.in/common/types.h>
#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <private/gl/Gradient.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            Gradient::Gradient(const linear_t & params)
            {
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

            Gradient::Gradient(const radial_t & params)
            {
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

            Gradient::~Gradient()
            {
            }

            void Gradient::set_start(float r, float g, float b, float a)
            {
                sStart.r    = r;
                sStart.g    = g;
                sStart.b    = b;
                sStart.a    = a;
            }

            void Gradient::set_stop(float r, float g, float b, float a)
            {
                sEnd.r      = r;
                sEnd.g      = g;
                sEnd.b      = b;
                sEnd.a      = a;
            }

            size_t Gradient::serial_size() const
            {
                return (bLinear) ? 12 * sizeof(float) : 16 * sizeof(float);
            }

            float *Gradient::serialize(float *buf) const
            {
                const float sa  = 1.0f - sStart.a;
                const float ea  = 1.0f - sEnd.a;

                buf[0]  = sStart.r * sa;
                buf[1]  = sStart.g * sa;
                buf[2]  = sStart.b * sa;
                buf[3]  = sa;

                buf[4]  = sEnd.r * ea;
                buf[5]  = sEnd.g * ea;
                buf[6]  = sEnd.b * ea;
                buf[7]  = ea;

                if (bLinear)
                {
                    buf[8]  = sLinear.x1;
                    buf[9]  = sLinear.y1;
                    buf[10] = sLinear.x2;
                    buf[11] = sLinear.y2;

                    return &buf[12];
                }
                else
                {
                    buf[8]  = sRadial.x1;
                    buf[9]  = sRadial.y1;
                    buf[10] = sRadial.x2;
                    buf[11] = sRadial.y2;

                    buf[12] = sRadial.r;
                    buf[13] = 0.0f;
                    buf[14] = 0.0f;
                    buf[15] = 0.0f;

                    return &buf[16];
                }
            }

            bool Gradient::linear() const
            {
                return bLinear;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
