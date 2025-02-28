/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 июл. 2022 г.
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

#include <lsp-plug.in/ws/version.h>

#include <lsp-plug.in/ws/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>

#include <private/win/WinDDGradient.h>
#include <private/win/WinDisplay.h>

#include <d2d1.h>
#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            WinDDGradient::WinDDGradient(ID2D1RenderTarget *dc, const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES & props)
            {
                pDC             = dc;
                pBrush          = NULL;
                sLinear         = props;
                bLinear         = true;

                vPoints[0].position = 0.0f;
                vPoints[0].color.r  = 0.0f;
                vPoints[0].color.g  = 0.0f;
                vPoints[0].color.b  = 0.0f;
                vPoints[0].color.a  = 1.0f;

                vPoints[1].position = 1.0f;
                vPoints[1].color.r  = 1.0f;
                vPoints[1].color.g  = 1.0f;
                vPoints[1].color.b  = 1.0f;
                vPoints[1].color.a  = 0.0f;
            }

            WinDDGradient::WinDDGradient(ID2D1RenderTarget *dc, const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES & props)
            {
                pDC             = dc;
                pBrush          = NULL;
                sRadial         = props;
                bLinear         = false;

                vPoints[0].position = 0.0f;
                vPoints[0].color.r  = 0.0f;
                vPoints[0].color.g  = 0.0f;
                vPoints[0].color.b  = 0.0f;
                vPoints[0].color.a  = 1.0f;

                vPoints[1].position = 1.0f;
                vPoints[1].color.r  = 1.0f;
                vPoints[1].color.g  = 1.0f;
                vPoints[1].color.b  = 1.0f;
                vPoints[1].color.a  = 0.0f;
            }

            WinDDGradient::~WinDDGradient()
            {
                drop_brush();
            }

            void WinDDGradient::drop_brush()
            {
                if (pBrush != NULL)
                {
                    pBrush->Release();
                    pBrush  = NULL;
                }
            }

            void WinDDGradient::set_start(float r, float g, float b, float a)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[0];
                p->color.r  = r;
                p->color.g  = g;
                p->color.b  = b;
                p->color.a  = 1.0f - a;
            }

            void WinDDGradient::set_start(const Color &c)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[0];
                c.get_rgbo(p->color.r, p->color.g, p->color.b, p->color.a);
            }

            void WinDDGradient::set_start(const Color &c, float a)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[0];
                c.get_rgb(p->color.r, p->color.g, p->color.b);
                p->color.a  = 1.0f - a;
            }

            void WinDDGradient::set_stop(float r, float g, float b, float a)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[1];
                p->color.r  = r;
                p->color.g  = g;
                p->color.b  = b;
                p->color.a  = 1.0f - a;
            }

            void WinDDGradient::set_stop(const Color &c)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[1];
                c.get_rgbo(p->color.r, p->color.g, p->color.b, p->color.a);
            }

            void WinDDGradient::set_stop(const Color &c, float a)
            {
                drop_brush();
                D2D1_GRADIENT_STOP *p = &vPoints[1];
                c.get_rgb(p->color.r, p->color.g, p->color.b);
                p->color.a  = 1.0f - a;
            }

            ID2D1Brush *WinDDGradient::get_brush()
            {
                if (pBrush != NULL)
                    return pBrush;

                // Create gradient stop collection
                ID2D1GradientStopCollection *list = NULL;
                HRESULT hr = pDC->CreateGradientStopCollection(
                    vPoints,
                    2,
                    D2D1_GAMMA_2_2,
                    D2D1_EXTEND_MODE_CLAMP,
                    &list);
                if (FAILED(hr) || (list == NULL))
                    return NULL;
                lsp_finally{ list->Release(); };

                // Create brush
                if (bLinear)
                {
                    ID2D1LinearGradientBrush *lg_brush = NULL;
                    hr = pDC->CreateLinearGradientBrush(sLinear, list, &lg_brush);
                    pBrush = lg_brush;
                }
                else
                {
                    ID2D1RadialGradientBrush *rg_brush = NULL;
                    hr = pDC->CreateRadialGradientBrush(sRadial, list, &rg_brush);
                    pBrush = rg_brush;
                }

                return pBrush;
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */


