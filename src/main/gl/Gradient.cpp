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
            Gradient::Gradient(const gl::linear_gradient_t & params)
            {
                sLinear     = params;
                bLinear     = true;
            }

            Gradient::Gradient(const gl::radial_gradient_t & params)
            {
                sRadial     = params;
                bLinear     = false;
            }

            Gradient::~Gradient()
            {
            }

            void Gradient::set_start(float r, float g, float b, float a)
            {
                gl::color_t & start = sBase.start;
                start.r     = r;
                start.g     = g;
                start.b     = b;
                start.a     = a;
            }

            void Gradient::set_stop(float r, float g, float b, float a)
            {
                gl::color_t & end   = sBase.end;
                end.r       = r;
                end.g       = g;
                end.b       = b;
                end.a       = a;
            }

            size_t Gradient::serial_size() const
            {
                return (bLinear) ? 12 * sizeof(float) : 16 * sizeof(float);
            }

            float *Gradient::serialize(float *buf) const
            {
                const gl::color_t & start = sBase.start;
                const gl::color_t & end   = sBase.end;

                const float sa  = 1.0f - start.a;
                const float ea  = 1.0f - end.a;

                buf[0]  = start.r * sa;
                buf[1]  = start.g * sa;
                buf[2]  = start.b * sa;
                buf[3]  = sa;

                buf[4]  = end.r * ea;
                buf[5]  = end.g * ea;
                buf[6]  = end.b * ea;
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

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
