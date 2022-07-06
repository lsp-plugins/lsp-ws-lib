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
#include <private/win/WinDDSurface.h>
#include <private/win/WinDisplay.h>

#include <d2d1.h>
#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            static inline D2D_COLOR_F d2d_color(const Color &color)
            {
                D2D_COLOR_F c;
                color.get_rgbo(c.r, c.g, c.b, c.a);
                return c;
            }

            template <class T>
                static inline void safe_release(T * &obj)
                {
                    if (obj != NULL)
                    {
                        obj->Release();
                        obj = NULL;
                    }
                }

            WinDDSurface::WinDDSurface(WinDisplay *dpy, HWND hwnd, size_t width, size_t height)
            {
                pDisplay    = dpy;
                hWindow     = hwnd;
                pDC         = NULL;

                nWidth      = width;
                nHeight     = height;
                nType       = ST_DDRAW;
            }

            WinDDSurface::WinDDSurface(WinDisplay *dpy, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                pDisplay    = dpy;
                hWindow     = NULL;
                pDC         = NULL;
            }

            WinDDSurface::~WinDDSurface()
            {
            }

            void WinDDSurface::destroy()
            {
                safe_release(pDC);
            }

            void WinDDSurface::begin()
            {
                if (hWindow == NULL)
                    return;

                // Create render target if necessary
                if (pDC == NULL)
                {
                    FLOAT dpi_x, dpi_y;
                    D2D1_RENDER_TARGET_PROPERTIES prop;
                    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProp;

                    pDisplay->d2d_factory()->GetDesktopDpi(&dpi_x, &dpi_y);

                    prop.type                   = D2D1_RENDER_TARGET_TYPE_DEFAULT;
                    prop.pixelFormat.format     = DXGI_FORMAT_UNKNOWN;
                    prop.pixelFormat.alphaMode  = D2D1_ALPHA_MODE_UNKNOWN; // D2D1_ALPHA_MODE_STRAIGHT;
                    prop.dpiX                   = dpi_x;
                    prop.dpiY                   = dpi_y;
                    prop.usage                  = D2D1_RENDER_TARGET_USAGE_NONE;
                    prop.minLevel               = D2D1_FEATURE_LEVEL_DEFAULT;

                    hwndProp.hwnd               = hWindow;
                    hwndProp.pixelSize.width    = nWidth;
                    hwndProp.pixelSize.height   = nHeight;
                    hwndProp.presentOptions     = D2D1_PRESENT_OPTIONS_NONE;

                    ID2D1HwndRenderTarget *ht   = NULL;
                    HRESULT res                 = pDisplay->d2d_factory()->CreateHwndRenderTarget(prop, hwndProp, &ht);
                    if (res != S_OK)
                    {
                        lsp_error("Error creating HWND render target: 0x%08lx", long(res));
                        return;
                    }

                    pDC         = ht;
                }

                pDC->BeginDraw();
            }

            void WinDDSurface::end()
            {
                if (hWindow == NULL)
                    return;
                if (pDC == NULL)
                    return;

                HRESULT hr = pDC->EndDraw();
                if (FAILED(hr) || (hr == HRESULT(D2DERR_RECREATE_TARGET)))
                    safe_release(pDC);
            }

            void WinDDSurface::sync_size()
            {
                if (hWindow == NULL)
                    return;

                RECT rc;
                D2D1_SIZE_U size;
                GetClientRect(hWindow, &rc);

                nWidth      = rc.right - rc.left;
                nHeight     = rc.bottom - rc.top;

                if (pDC != NULL)
                {
                    ID2D1HwndRenderTarget *ht   = static_cast<ID2D1HwndRenderTarget *>(pDC);

                    size.width      = nWidth;
                    size.height     = nHeight;
                    ht->Resize(&size);
                }
            }

            ISurface *WinDDSurface::create(size_t width, size_t height)
            {
                return NULL;
            }

            ISurface *WinDDSurface::create_copy()
            {
                return NULL;
            }

            IGradient *WinDDSurface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                if (pDC == NULL)
                    return NULL;
                return new WinDDGradient(
                    pDC,
                    D2D1::LinearGradientBrushProperties(
                        D2D1::Point2F(x0, y0),
                        D2D1::Point2F(x1, y1)));
            }

            IGradient *WinDDSurface::radial_gradient
            (
                float cx0, float cy0,
                float cx1, float cy1,
                float r
            )
            {
                if (pDC == NULL)
                    return NULL;
                return new WinDDGradient(
                    pDC,
                    D2D1::RadialGradientBrushProperties(
                        D2D1::Point2F(cx1, cy1),
                        D2D1::Point2F(cx0 - cx1, cy0 - cy1),
                        r, r
                    ));
            }

            void WinDDSurface::draw(ISurface *s, float x, float y)
            {
            }

            void WinDDSurface::draw(ISurface *s, float x, float y, float sx, float sy)
            {
            }

            void WinDDSurface::draw(ISurface *s, const ws::rectangle_t *r)
            {
            }

            void WinDDSurface::draw_alpha(ISurface *s, float x, float y, float sx, float sy, float a)
            {
            }

            void WinDDSurface::draw_rotate_alpha(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
            }

            void WinDDSurface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh)
            {
            }

            void WinDDSurface::draw_rounded_rectangle(const D2D_RECT_F &rect, size_t mask, float radius, float line_width, ID2D1Brush *brush)
            {
                // Simple geometry?
                if ((!(mask & SURFMASK_ALL_CORNER)) || (radius <= 0.0f))
                {
                    if (line_width < 0.0f)
                        pDC->FillRectangle(&rect, brush);
                    else
                        pDC->DrawRectangle(&rect, brush, line_width, NULL);
                }

                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally( safe_release(g); );

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally( safe_release(s); );
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Generate geometry for the rounded rectangle
                D2D1_ARC_SEGMENT arc = D2D1::ArcSegment(
                    D2D1::Point2F(0.0f, 0.0f),
                    D2D1::SizeF(radius, radius),
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    D2D1_ARC_SIZE_SMALL);

                D2D1_FIGURE_BEGIN mode = (line_width < 0.0f) ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW;
                if (mask & SURFMASK_LT_CORNER)
                {
                    s->BeginFigure(
                        D2D1::Point2F(rect.left, rect.top + radius),
                        mode);
                    arc.point   = D2D1::Point2F(rect.left + radius, rect.top);
                    s->AddArc(&arc);
                }
                else
                    s->BeginFigure(
                        D2D1::Point2F(rect.left, rect.top),
                        mode);

                if (mask & SURFMASK_RT_CORNER)
                {
                    s->AddLine(D2D1::Point2F(rect.right - radius, rect.top));
                    arc.point   = D2D1::Point2F(rect.right, rect.top + radius);
                    s->AddArc(&arc);
                }
                else
                    s->AddLine(D2D1::Point2F(rect.right, rect.top));

                if (mask & SURFMASK_RB_CORNER)
                {
                    s->AddLine(D2D1::Point2F(rect.right, rect.bottom - radius));
                    arc.point   = D2D1::Point2F(rect.right - radius, rect.bottom);
                    s->AddArc(&arc);
                }
                else
                    s->AddLine(D2D1::Point2F(rect.right, rect.bottom));

                if (mask & SURFMASK_LB_CORNER)
                {
                    s->AddLine(D2D1::Point2F(rect.left + radius, rect.bottom));
                    arc.point   = D2D1::Point2F(rect.left, rect.bottom - radius);
                    s->AddArc(&arc);
                }
                else
                    s->AddLine(D2D1::Point2F(rect.left, rect.bottom));

                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                if (line_width < 0.0f)
                    pDC->FillGeometry(g, brush, NULL);
                else
                    pDC->DrawGeometry(g, brush, line_width, NULL);
            }

            void WinDDSurface::wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                D2D_RECT_F rect;
                float hw    = line_width * 0.5f;
                rect.left   = left + hw;
                rect.top    = top  + hw;
                rect.right  = left + width  - hw;
                rect.bottom = top  + height - hw;
                radius     -= hw;

                draw_rounded_rectangle(rect, mask, radius, line_width, brush);
            }

            void WinDDSurface::wire_rect(const Color &c, size_t mask, float radius, const rectangle_t *r, float line_width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                D2D_RECT_F rect;
                float hw    = line_width * 0.5f;
                rect.left   = r->nLeft + hw;
                rect.top    = r->nTop  + hw;
                rect.right  = r->nLeft + r->nWidth  - hw;
                rect.bottom = r->nTop  + r->nHeight - hw;
                radius     -= hw;

                draw_rounded_rectangle(rect, mask, radius, line_width, brush);
            }

            void WinDDSurface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                D2D_RECT_F rect;
                float hw    = line_width * 0.5f;
                rect.left   = left + hw;
                rect.top    = top  + hw;
                rect.right  = left + width  - hw;
                rect.bottom = top  + height - hw;
                radius     -= hw;

                draw_rounded_rectangle(rect, mask, radius, line_width, brush);
            }

            void WinDDSurface::wire_rect(IGradient *g, size_t mask, float radius, const rectangle_t *r, float line_width)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                D2D_RECT_F rect;
                float hw    = line_width * 0.5f;
                rect.left   = r->nLeft + hw;
                rect.top    = r->nTop  + hw;
                rect.right  = r->nLeft + r->nWidth  - hw;
                rect.bottom = r->nTop  + r->nHeight - hw;
                radius     -= hw;

                draw_rounded_rectangle(rect, mask, radius, line_width, brush);
            }

            void WinDDSurface::fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                D2D_RECT_F rect;
                rect.left   = left;
                rect.top    = top;
                rect.right  = left + width;
                rect.bottom = top  + height;

                draw_rounded_rectangle(rect, mask, radius, -1.0f, brush);
            }

            void WinDDSurface::fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                D2D_RECT_F rect;
                rect.left   = r->nLeft;
                rect.top    = r->nTop;
                rect.right  = r->nLeft + r->nWidth;
                rect.bottom = r->nTop  + r->nHeight;

                draw_rounded_rectangle(rect, mask, radius, -1.0f, brush);
            }

            void WinDDSurface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                D2D_RECT_F rect;
                rect.left   = left;
                rect.top    = top;
                rect.right  = left + width;
                rect.bottom = top  + height;

                draw_rounded_rectangle(rect, mask, radius, -1.0f, brush);
            }

            void WinDDSurface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                D2D_RECT_F rect;
                rect.left   = r->nLeft;
                rect.top    = r->nTop;
                rect.right  = r->nLeft + r->nWidth;
                rect.bottom = r->nTop  + r->nHeight;

                draw_rounded_rectangle(rect, mask, radius, -1.0f, brush);
            }

            void WinDDSurface::fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally( safe_release(g); );

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally( safe_release(s); );
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the sector
                s->BeginFigure(
                    D2D1::Point2F(cx, cy),
                    D2D1_FIGURE_BEGIN_FILLED);
                s->AddLine(D2D1::Point2F(cx + radius * cosf(angle1), cy + radius * sinf(angle1)));
                s->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(cx + radius * cosf(angle2), cy + radius * sinf(angle2)),
                        D2D1::SizeF(radius, radius),
                        0.0f,
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        (fabs(angle2 - angle1) >= M_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL)
                    );
                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, brush, NULL);
            }

            void WinDDSurface::draw_triangle(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally( safe_release(g); );

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally( safe_release(s); );
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the sector
                s->BeginFigure(
                    D2D1::Point2F(x0, y0),
                    D2D1_FIGURE_BEGIN_FILLED);
                s->AddLine(D2D1::Point2F(x1, y1));
                s->AddLine(D2D1::Point2F(x2, y2));
                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, brush, NULL);
            }

            void WinDDSurface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                draw_triangle(brush, x0, y0, x1, y1, x2, y2);
            }

            void WinDDSurface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                draw_triangle(brush, x0, y0, x1, y1, x2, y2);
            }

            void WinDDSurface::fill_circle(const Color & c, float x, float y, float r)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                pDC->FillEllipse(
                    D2D1::Ellipse(D2D1::Point2F(x, y), r, r),
                    brush);
            }

            void WinDDSurface::fill_circle(IGradient *g, float x, float y, float r)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                pDC->FillEllipse(
                    D2D1::Ellipse(D2D1::Point2F(x, y), r, r),
                    brush);
            }

            void WinDDSurface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally( safe_release(g); );

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally( safe_release(s); );
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the arc
                r = lsp_max(0.0f, r - width * 0.5f);
                s->BeginFigure(
                    D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)),
                    D2D1_FIGURE_BEGIN_HOLLOW);
                s->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(x + r * cosf(a2), y + r * sinf(a2)),
                        D2D1::SizeF(r, r),
                        0.0f,
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        (fabs(a2 - a1) >= M_PI) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL)
                    );
                s->EndFigure(D2D1_FIGURE_END_OPEN);
                s->Close();

                // Draw the geometry
                pDC->DrawGeometry(g, brush, width);
            }

            void WinDDSurface::line(const Color &c, float x0, float y0, float x1, float y1, float width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                pDC->DrawLine(
                    D2D1::Point2F(x0, y0),
                    D2D1::Point2F(x1, y1),
                    brush, width, NULL);
            }

            void WinDDSurface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                if ((pDC == NULL) || (g == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;
                pDC->DrawLine(
                    D2D1::Point2F(x0, y0),
                    D2D1::Point2F(x1, y1),
                    brush, width, NULL);
            }

            void WinDDSurface::parametric_line(const Color &color, float a, float b, float c, float width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                if (fabs(a) > fabs(b))
                    pDC->DrawLine(
                        D2D1::Point2F(- c / a, 0.0f),
                        D2D1::Point2F(-(c + b*nHeight)/a, nHeight),
                        brush, width, NULL);
                else
                    pDC->DrawLine(
                        D2D1::Point2F(0.0f, - c / b),
                        D2D1::Point2F(nWidth, -(c + a*nWidth)/b),
                        brush, width, NULL);
            }

            void WinDDSurface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally( safe_release(brush); );

                if (fabs(a) > fabs(b))
                    pDC->DrawLine(
                        D2D1::Point2F(roundf(-(c + b*top)/a), roundf(top)),
                        D2D1::Point2F(roundf(-(c + b*bottom)/a), roundf(bottom)),
                        brush, width, NULL);
                else
                    pDC->DrawLine(
                        D2D1::Point2F(roundf(left), roundf(-(c + a*left)/b)),
                        D2D1::Point2F(roundf(right), roundf(-(c + a*right)/b)),
                        brush, width, NULL);
            }

            void WinDDSurface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
            }

            void WinDDSurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
            }

            void WinDDSurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
            }

            void WinDDSurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
            }

            void WinDDSurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
            }

            bool WinDDSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                return false;
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
            {
                return false;
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text)
            {
                return false;
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first)
            {
                return false;
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                return false;
            }

            void WinDDSurface::clear(const Color &color)
            {
                if (pDC == NULL)
                    return;

                D2D_COLOR_F c;
                color.get_rgba(c.r, c.g, c.b, c.a);
                pDC->Clear(&c);
            }

            void WinDDSurface::clear_rgb(uint32_t color)
            {
                if (pDC == NULL)
                    return;

                D2D_COLOR_F c;
                c.r     = ((color >> 16) & 0xff) / 255.0f;
                c.g     = ((color >> 8) & 0xff) / 255.0f;
                c.b     = (color & 0xff) / 255.0f;
                c.a     = ((color >> 24) & 0xff) / 255.0f;
                pDC->Clear(&c);
            }

            void WinDDSurface::clear_rgba(uint32_t color)
            {
                D2D_COLOR_F c;
                c.r     = ((color >> 16) & 0xff) / 255.0f;
                c.g     = ((color >> 8) & 0xff) / 255.0f;
                c.b     = (color & 0xff) / 255.0f;
                c.a     = 0.0f;
                pDC->Clear(&c);
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text)
            {
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first)
            {
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text)
            {
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first)
            {
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
            }

            void WinDDSurface::fill_frame(const Color &color,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih
            )
            {
            }

            void WinDDSurface::fill_frame(const Color &color, const ws::rectangle_t *out, const ws::rectangle_t *in)
            {
            }

            void WinDDSurface::fill_round_frame(
                const Color &color,
                float radius, size_t flags,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih
            )
            {
            }

            void WinDDSurface::fill_round_frame(
                const Color &color, float radius, size_t flags,
                const ws::rectangle_t *out, const ws::rectangle_t *in
            )
            {
            }

            void WinDDSurface::clip_begin(float x, float y, float w, float h)
            {
            }

            void WinDDSurface::clip_begin(const ws::rectangle_t *area)
            {
            }

            void WinDDSurface::clip_end()
            {
            }

            bool WinDDSurface::get_antialiasing()
            {
                return false;
            }

            bool WinDDSurface::set_antialiasing(bool set)
            {
                return false;
            }

            surf_line_cap_t WinDDSurface::get_line_cap()
            {
                return SURFLCAP_BUTT;
            }

            surf_line_cap_t WinDDSurface::set_line_cap(surf_line_cap_t lc)
            {
                return SURFLCAP_BUTT;
            }

            void *WinDDSurface::start_direct()
            {
                return NULL;
            }

            void WinDDSurface::end_direct()
            {
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

