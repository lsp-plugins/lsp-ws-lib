/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                public:
                    typedef struct linear_t
                    {
                        float x1;
                        float y1;
                        float x2;
                        float y2;
                    } linear_t;

                    typedef struct radial_t
                    {
                        float x1;
                        float y1;
                        float x2;
                        float y2;
                        float r;
                    } radial_t;

                protected:
                    typedef struct color_t
                    {
                        float r, g, b, a;
                    } color_t;

                protected:
                    cairo_pattern_t    *pCP;
                    union
                    {
                        linear_t    sLinear;
                        radial_t    sRadial;
                    };
                    color_t             sStart;
                    color_t             sEnd;
                    bool                bLinear;

                protected:
                    void drop_pattern();

                public:
                    explicit X11CairoGradient(const linear_t & params);
                    explicit X11CairoGradient(const radial_t & params);
                    X11CairoGradient(const X11CairoGradient &) = delete;
                    X11CairoGradient(X11CairoGradient &&) = delete;
                    virtual ~X11CairoGradient() override;

                    X11CairoGradient & operator = (const X11CairoGradient &) = delete;
                    X11CairoGradient & operator = (X11CairoGradient &&) = delete;

                public:
                    virtual void set_start(float r, float g, float b, float a) override;
                    virtual void set_stop(float r, float g, float b, float a) override;

                public:
                    void apply(cairo_t *cr);
            };

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(USE_LIBX11) && defined(USE_LIBCAIRO) */

#endif /* UI_X11_X11CAIROGRADIENT_H_ */
