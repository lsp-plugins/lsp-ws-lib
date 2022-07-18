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
#include <wincodec.h>
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

            static inline D2D1_RECT_F d2d_rect(float x, float y, float w, float h)
            {
                D2D1_RECT_F r;
                r.left      = x;
                r.top       = y;
                r.right     = x + w;
                r.bottom    = y + h;
                return r;
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
                pDisplay        = dpy;
                hWindow         = hwnd;
                pDC             = NULL;

                nWidth          = width;
                nHeight         = height;
                nType           = ST_DDRAW;
            #ifdef LSP_DEBUG
                nClipping       = 0;
            #endif /* LSP_DEBUG */
            }

            WinDDSurface::WinDDSurface(WinDisplay *dpy, ID2D1RenderTarget *dc, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                pDisplay        = dpy;
                hWindow         = NULL;
                pDC             = dc;

            #ifdef LSP_DEBUG
                nClipping       = 0;
            #endif /* LSP_DEBUG */
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
                // Create render target if necessary
                if ((pDC == NULL) && (hWindow != NULL) && (nType == ST_DDRAW))
                {
                    FLOAT dpi_x, dpi_y;
                    D2D1_RENDER_TARGET_PROPERTIES prop;
                    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProp;

                    pDisplay->d2d_factory()->GetDesktopDpi(&dpi_x, &dpi_y);

                    prop.type                   = D2D1_RENDER_TARGET_TYPE_DEFAULT;
                    prop.pixelFormat.format     = DXGI_FORMAT_B8G8R8A8_UNORM;
                    prop.pixelFormat.alphaMode  = D2D1_ALPHA_MODE_PREMULTIPLIED; // D2D1_ALPHA_MODE_STRAIGHT;
                    prop.dpiX                   = dpi_x;
                    prop.dpiY                   = dpi_y;
                    prop.usage                  = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
                    prop.minLevel               = D2D1_FEATURE_LEVEL_DEFAULT;

                    hwndProp.hwnd               = hWindow;
                    hwndProp.pixelSize.width    = nWidth;
                    hwndProp.pixelSize.height   = nHeight;
                    hwndProp.presentOptions     = D2D1_PRESENT_OPTIONS_NONE;

                    ID2D1HwndRenderTarget *ht   = NULL;
                    HRESULT hr                  = pDisplay->d2d_factory()->CreateHwndRenderTarget(prop, hwndProp, &ht);
                    if (FAILED(hr))
                    {
                        lsp_error("Error creating HWND render target: 0x%08lx", long(hr));
                        return;
                    }

                    pDC         = ht;
                }

                if (pDC != NULL)
                {
                    pDC->BeginDraw();
                    pDC->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
            }

            void WinDDSurface::end()
            {
                if (pDC == NULL)
                    return;

            #ifdef LSP_DEBUG
                if (nClipping > 0)
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
            #endif /* LSP_DEBUG */

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

            void WinDDSurface::clear(const Color &color)
            {
                if (pDC == NULL)
                    return;

                D2D_COLOR_F c;
                color.get_rgbo(c.r, c.g, c.b, c.a);
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
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
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
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                lsp_finally{ safe_release(brush); };

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
                IGradient *gr,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                if ((pDC == NULL) || (gr == NULL))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(gr)->get_brush();
                if (brush == NULL)
                    return;

                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the sector
                if (fabs(a1) > fabs(b1))
                {
                    s->BeginFigure(
                        D2D1::Point2F(ssize_t(-(c1 + b1*top)/a1), ssize_t(top)),
                        D2D1_FIGURE_BEGIN_FILLED);
                    s->AddLine(D2D1::Point2F(ssize_t(-(c1 + b1*bottom)/a1), ssize_t(bottom)));
                }
                else
                {
                    s->BeginFigure(
                        D2D1::Point2F(ssize_t(left), ssize_t(-(c1 + a1*left)/b1)),
                        D2D1_FIGURE_BEGIN_FILLED);
                    s->AddLine(D2D1::Point2F(ssize_t(right), ssize_t(-(c1 + a1*right)/b1)));
                }

                if (fabs(a2) > fabs(b2))
                {
                    s->AddLine(D2D1::Point2F(ssize_t(-(c2 + b2*bottom)/a2), ssize_t(bottom)));
                    s->AddLine(D2D1::Point2F(ssize_t(-(c2 + b2*top)/a2), ssize_t(top)));
                }
                else
                {
                    s->AddLine(D2D1::Point2F(ssize_t(right), ssize_t(-(c2 + a2*right)/b2)));
                    s->AddLine(D2D1::Point2F(ssize_t(left), ssize_t(-(c2 + a2*left)/b2)));
                }

                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, brush);
            }

            void WinDDSurface::draw_polygon(ID2D1Brush *brush, const float *x, const float *y, size_t n, float width)
            {
                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the polygon with lines
                s->BeginFigure(D2D1::Point2F(x[0], y[0]), (width < 0.0f) ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
                for (size_t i=1; i<n; ++i)
                    s->AddLine(D2D1::Point2F(x[i], y[i]));
                s->EndFigure((width < 0.0f) ?  D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
                s->Close();

                // Draw the geometry
                if (width < 0.0f)
                    pDC->FillGeometry(g, brush);
                else
                    pDC->DrawGeometry(g, brush, width);
            }

            void WinDDSurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
                if ((pDC == NULL) || (n < 2))
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                draw_polygon(brush, x, y, n, -1.0f);
            }

            void WinDDSurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
                if ((pDC == NULL) || (gr == NULL) || (n < 2))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(gr)->get_brush();
                if (brush == NULL)
                    return;

                draw_polygon(brush, x, y, n, -1.0f);
            }

            void WinDDSurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
                if ((pDC == NULL) || (n < 2))
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                draw_polygon(brush, x, y, n, width);
            }

            void WinDDSurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                if ((pDC == NULL) || (n < 2))
                    return;

                // Create brushes
                ID2D1SolidColorBrush *f_brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(fill), &f_brush)))
                    return;
                lsp_finally{ safe_release(f_brush); };
                ID2D1SolidColorBrush *w_brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(wire), &w_brush)))
                    return;
                lsp_finally{ safe_release(w_brush); };

                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the polygon with lines
                s->BeginFigure(D2D1::Point2F(x[0], y[0]), D2D1_FIGURE_BEGIN_FILLED);
                for (size_t i=1; i<n; ++i)
                    s->AddLine(D2D1::Point2F(x[i], y[i]));
                s->EndFigure(D2D1_FIGURE_END_OPEN);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, f_brush);
                pDC->DrawGeometry(g, w_brush, width);
            }

            void WinDDSurface::draw_negative_arc(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pDisplay->d2d_factory()->CreatePathGeometry(&g)))
                    return;
                lsp_finally{ safe_release(g); };

                // Create sink
                ID2D1GeometrySink *s = NULL;
                if (FAILED(g->Open(&s)))
                    return;
                lsp_finally{ safe_release(s); };
                s->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

                // Draw the negative arc
                float radius = fabs(x1 - x0 + y1 - y0);
                s->BeginFigure(
                    D2D1::Point2F(x0, y0),
                    D2D1_FIGURE_BEGIN_FILLED);
                s->AddLine(D2D1::Point2F(x1, y1));
                s->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(x2, y2),
                        D2D1::SizeF(radius, radius),
                        0.0f,
                        D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                        D2D1_ARC_SIZE_SMALL)
                    );
                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, brush, NULL);
            }

            void WinDDSurface::fill_frame(
                const Color &color,
                size_t flags, float radius,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                if (pDC == NULL)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                // Draw the frame
                float fxe = fx + fw, fye = fy + fh, ixe = ix + iw, iye = iy + ih;

                if ((ix >= fxe) || (ixe < fx) || (iy >= fye) || (iye < fy))
                {
                    pDC->FillRectangle(d2d_rect(fx, fy, fw, fh), brush);
                    return;
                }
                else if ((ix <= fx) && (ixe >= fxe) && (iy <= fy) && (iye >= fye))
                    return;

                if (ix <= fx)
                {
                    if (iy <= fy)
                    {
                        pDC->FillRectangle(d2d_rect(ixe, fy, fxe - ixe, iye - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                    else if (iye >= fye)
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(ixe, iy, fxe - ixe, fye - iy), brush);
                    }
                    else
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(ixe, iy, fxe - ixe, ih), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                }
                else if (ixe >= fxe)
                {
                    if (iy <= fy)
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, ix - fx, iye - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                    else if (iye >= fye)
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iy, ix - fx, fye - iy), brush);
                    }
                    else
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iy, ix - fx, ih), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                }
                else
                {
                    if (iy <= fy)
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, ix - fx, iye - fy), brush);
                        pDC->FillRectangle(d2d_rect(ixe, fy, fxe - ixe, iye - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                    else if (iye >= fye)
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iy, ix - fx, fye - iy), brush);
                        pDC->FillRectangle(d2d_rect(ixe, iy, fxe - ixe, fye - iy), brush);
                    }
                    else
                    {
                        pDC->FillRectangle(d2d_rect(fx, fy, fw, iy - fy), brush);
                        pDC->FillRectangle(d2d_rect(fx, iy, ix - fx, ih), brush);
                        pDC->FillRectangle(d2d_rect(ixe, iy, fxe - ixe, ih), brush);
                        pDC->FillRectangle(d2d_rect(fx, iye, fw, fye - iye), brush);
                    }
                }

                // Ensure that there are no corners
                if ((radius <= 0.0) || (!(flags & SURFMASK_ALL_CORNER)))
                    return;

                // Can draw corners?
                float minw = 0.0f;
                minw += (flags & SURFMASK_L_CORNER) ? radius : 0.0;
                minw += (flags & SURFMASK_R_CORNER) ? radius : 0.0;
                if (iw < minw)
                    return;

                float minh = 0.0f;
                minh += (flags & SURFMASK_T_CORNER) ? radius : 0.0;
                minh += (flags & SURFMASK_B_CORNER) ? radius : 0.0;
                if (ih < minh)
                    return;

                // Draw corners
                if (flags & SURFMASK_LT_CORNER)
                    draw_negative_arc(
                        brush,
                        ix, iy,
                        ix + radius, iy,
                        ix, iy + radius);
                if (flags & SURFMASK_RT_CORNER)
                    draw_negative_arc(
                        brush,
                        ixe, iy,
                        ixe, iy + radius,
                        ixe - radius, iy);
                if (flags & SURFMASK_LB_CORNER)
                    draw_negative_arc(
                        brush,
                        ix, iye,
                        ix, iye - radius,
                        ix + radius, iye);
                if (flags & SURFMASK_RB_CORNER)
                    draw_negative_arc(
                        brush,
                        ixe, iye,
                        ixe - radius, iye,
                        ixe, iye - radius);
            }

            void WinDDSurface::clip_begin(float x, float y, float w, float h)
            {
                if (pDC == NULL)
                    return;

                // Create the clipping layer
                ID2D1Layer *layer = NULL;
                if (!SUCCEEDED(pDC->CreateLayer(NULL, &layer)))
                    return;
                lsp_finally{ safe_release(layer); };

                // Apply the layer
                pDC->PushLayer(
                    D2D1::LayerParameters(D2D1::RectF(x, y, x+w, y+h)),
                    layer);

                #ifdef LSP_DEBUG
                ++ nClipping;
                #endif /* LSP_DEBUG */
            }

            void WinDDSurface::clip_end()
            {
                if (pDC == NULL)
                    return;

            #ifdef LSP_DEBUG
                if (nClipping <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                -- nClipping;
            #endif /* LSP_DEBUG */

                pDC->PopLayer();
            }

            IDisplay *WinDDSurface::display()
            {
                return pDisplay;
            }

            ISurface *WinDDSurface::create(size_t width, size_t height)
            {
                if (pDC == NULL)
                    return NULL;

                D2D1_SIZE_F desiredSize = D2D1::SizeF(width, height);
                D2D1_SIZE_U desiredPixelSize = D2D1::SizeU(width, height);
                D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED);

                ID2D1BitmapRenderTarget *dc = NULL;
                HRESULT hr = pDC->CreateCompatibleRenderTarget(
                    &desiredSize,
                    &desiredPixelSize,
                    &pixelFormat,
                    D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE, // options
                    &dc);
                if (FAILED(hr))
                    return NULL;

                return new WinDDSurface(pDisplay, dc, width, height);
            }

            ISurface *WinDDSurface::create_copy()
            {
                if ((pDC == NULL) || (type() != ST_IMAGE))
                    return NULL;

                // Get the bitmap of the surface
                ID2D1BitmapRenderTarget *sdc= static_cast<ID2D1BitmapRenderTarget *>(pDC);
                ID2D1Bitmap *bm             = NULL;
                if (FAILED(sdc->GetBitmap(&bm)))
                    return NULL;
                lsp_finally{ safe_release(bm); };

                // Create new render target
                D2D1_SIZE_F desiredSize = D2D1::SizeF(nWidth, nHeight);
                D2D1_SIZE_U desiredPixelSize = D2D1::SizeU(nWidth, nHeight);
                D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED);

                ID2D1BitmapRenderTarget *dc = NULL;
                HRESULT hr = sdc->CreateCompatibleRenderTarget(
                    &desiredSize,
                    &desiredPixelSize,
                    &pixelFormat,
                    D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
                    &dc);
                if (FAILED(hr))
                    return NULL;

                // Copy contents
                dc->BeginDraw();
                    dc->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
                    dc->DrawBitmap(
                        bm,
                        D2D1::RectF(0, 0, nWidth, nHeight),
                        1.0f,
                        D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                dc->EndDraw();

                return new WinDDSurface(pDisplay, dc, nWidth, nHeight);
            }

            void WinDDSurface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                if (pDC == NULL)
                    return;
                if (s->type() != ST_IMAGE)
                    return;

                // Get the source surface
                WinDDSurface *ws            = static_cast<WinDDSurface *>(s);
                if (ws->pDC == NULL)
                    return;

                ID2D1BitmapRenderTarget *dc = static_cast<ID2D1BitmapRenderTarget *>(ws->pDC);

                // Get the bitmap of the surface
                ID2D1Bitmap *bm             = NULL;
                if (FAILED(dc->GetBitmap(&bm)))
                    return;
                lsp_finally{ safe_release(bm); };

                // Draw the bitmap
                pDC->DrawBitmap(
                    bm,
                    D2D1::RectF(x, y, x + ws->nWidth * sx, y + ws->nHeight * sy),
                    1.0f - a,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            }

            void WinDDSurface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                if (pDC == NULL)
                    return;
                if (s->type() != ST_IMAGE)
                    return;

                // Get the source surface
                WinDDSurface *ws            = static_cast<WinDDSurface *>(s);
                if (ws->pDC == NULL)
                    return;
                ID2D1BitmapRenderTarget *dc = static_cast<ID2D1BitmapRenderTarget *>(ws->pDC);

                // Get the bitmap of the surface
                ID2D1Bitmap *bm             = NULL;
                if (FAILED(dc->GetBitmap(&bm)))
                    return;
                lsp_finally{ safe_release(bm); };

                // Draw the bitmap
                D2D1_MATRIX_3X2_F m;
                pDC->GetTransform(&m);
                pDC->SetTransform(D2D1::Matrix3x2F::Rotation(ra * 180.0f / M_PI, D2D1::Point2F(x, y)));

                pDC->DrawBitmap(
                    bm,
                    D2D1::RectF(x, y, x + ws->nWidth * sx, y + ws->nHeight * sy),
                    1.0f - a,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
                pDC->SetTransform(&m);
            }

            void WinDDSurface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a)
            {
                if (pDC == NULL)
                    return;
                if (s->type() != ST_IMAGE)
                    return;

                // Get the source surface
                WinDDSurface *ws            = static_cast<WinDDSurface *>(s);
                if (ws->pDC == NULL)
                    return;
                ID2D1BitmapRenderTarget *dc = static_cast<ID2D1BitmapRenderTarget *>(ws->pDC);

                // Get the bitmap of the surface
                ID2D1Bitmap *bm             = NULL;
                if (FAILED(dc->GetBitmap(&bm)))
                    return;
                lsp_finally{ safe_release(bm); };

                // Create the clipping layer
                ID2D1Layer *layer = NULL;
                if (!SUCCEEDED(pDC->CreateLayer(NULL, &layer)))
                    return;
                lsp_finally{ safe_release(layer); };

                // Apply the clipping layer
                pDC->PushLayer(
                    D2D1::LayerParameters(D2D1::RectF(x, y, x+sw, y+sh)),
                    layer);

                // Draw the bitmap
                x -= sx;
                y -= sy;
                pDC->DrawBitmap(
                    bm,
                    D2D1::RectF(x, y, x + ws->nWidth, y + ws->nHeight),
                    1.0f - a,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

                // Pop the clipping layer
                pDC->PopLayer();
            }

            void WinDDSurface::draw_raw(
                const void *data, size_t width, size_t height, size_t stride,
                float x, float y, float sx, float sy, float a)
            {
                HRESULT hr;

                // Create WIC bitmap
                IWICBitmap *wic = NULL;
                pDisplay->wic_factory()->CreateBitmapFromMemory(
                    width, height,
                    GUID_WICPixelFormat32bppPBGRA,
                    stride,
                    height * stride,
                    static_cast<BYTE *>(const_cast<void *>(data)),
                    &wic);
                if ((FAILED(hr)) || (wic == NULL))
                    return;
                lsp_finally{ safe_release(wic); };

                // Create ID2D1Bitmap from WIC bitmap
                ID2D1Bitmap *src = NULL;
                hr = pDC->CreateBitmapFromWicBitmap(wic, NULL, &src);
                if ((FAILED(hr)) || (src == NULL))
                    return;
                lsp_finally{ safe_release(src); };

                // Draw the bitmap
                pDC->DrawBitmap(
                    src,
                    D2D1::RectF(x, y, x + width * sx, y + height * sy),
                    1.0f - a,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            }

            bool WinDDSurface::get_font_parameters(const Font &f, font_parameters_t *fp)
            {
                // Redirect call to the display
                return pDisplay->get_font_parameters(f, fp);
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                // Redirect call to the display
                return pDisplay->get_text_parameters(f, tp, text, first, last);
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if ((pDC == NULL) || (text == NULL))
                    return;

                LSPString tmp;
                if (tmp.set_utf8(text))
                    out_text(f, color, x, y, &tmp, 0, tmp.length());
            }

            bool WinDDSurface::try_out_text(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, const WCHAR *text, size_t length)
            {
                // Create text layout
                IDWriteTextLayout *tl   = pDisplay->create_text_layout(f, family, fc, ff, text, length);
                if (tl == NULL)
                    return false;
                lsp_finally{ safe_release(tl); };

                // Create brush
                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return false;
                lsp_finally{ safe_release(brush); };

                // Draw the text
                D2D1_TEXT_ANTIALIAS_MODE antialias = pDC->GetTextAntialiasMode();
                switch (f.antialias())
                {
                    case FA_DISABLED:
                        pDC->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
                        break;
                    case FA_ENABLED:
                        pDC->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
                        break;
                    case FA_DEFAULT:
                    default:
                        break;
                }
                pDC->DrawTextLayout(
                    D2D1::Point2F(x, y),
                    tl,
                    brush,
                    D2D1_DRAW_TEXT_OPTIONS_NONE);
                pDC->SetTextAntialiasMode(antialias);

                return true;
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (pDC == NULL)
                    return;
                const WCHAR *pText = (text != NULL) ? reinterpret_cast<const WCHAR *>(text->get_utf16(first, last)) : NULL;
                if (pText == NULL)
                    return;
                size_t length = text->range_length(first, last);

                // Obtain the font family
                LSPString family_name;
                WinDisplay::font_t *custom = NULL;
                IDWriteFontFamily *ff   = pDisplay->get_font_family(f, &family_name, &custom);
                lsp_finally{ safe_release(ff); };

                if (custom != NULL)
                {
                    if (try_out_text(custom->collection, custom->family, custom->wname, f, color, x, y, pText, length))
                        return;
                }
                if (ff != NULL)
                {
                    if (try_out_text(NULL, ff, family_name.get_utf16(), f, color, x, y, pText, length))
                        return;
                }
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if ((pDC == NULL) || (text == NULL))
                    return;

                LSPString tmp;
                if (!tmp.set_utf8(text))
                    return;

                return out_text_relative(f, color, x, y, dx, dy, &tmp, 0, tmp.length());
            }

            bool WinDDSurface::try_out_text_relative(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, float dx, float dy, const WCHAR *text, size_t length)
            {
                // Obtain font metrics
                DWRITE_FONT_METRICS fm;
                if (!pDisplay->get_font_metrics(f, ff, &fm))
                    return false;

                // Create text layout
                IDWriteTextLayout *tl   = pDisplay->create_text_layout(f, family, fc, ff, text, length);
                if (tl == NULL)
                    return false;
                lsp_finally{ safe_release(tl); };

                // Create brush
                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return false;
                lsp_finally{ safe_release(brush); };

                // Get text layout metrics and font metrics
                DWRITE_TEXT_METRICS tm;
                tl->GetMetrics(&tm);

                // Compute the text position
                float ratio     = f.size() / float(fm.designUnitsPerEm);
                float height    = (fm.ascent + fm.descent + fm.lineGap) * ratio;
                float xbearing  = (f.italic()) ? sinf(0.033f * M_PI) * height : 0.0f;
                float r_w       = tm.width;
                float r_h       = fm.capHeight * ratio;
                float fx        = x - xbearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx;
                float fy        = y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy;

                // Draw the text
                D2D1_TEXT_ANTIALIAS_MODE antialias = pDC->GetTextAntialiasMode();
                switch (f.antialias())
                {
                    case FA_DISABLED:
                        pDC->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
                        break;
                    case FA_ENABLED:
                        pDC->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
                        break;
                    case FA_DEFAULT:
                    default:
                        break;
                }

                pDC->DrawTextLayout(
                    D2D1::Point2F(fx, fy),
                    tl,
                    brush,
                    D2D1_DRAW_TEXT_OPTIONS_NONE);
                pDC->SetTextAntialiasMode(antialias);

                return true;
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (pDC == NULL)
                    return;
                const WCHAR *pText = (text != NULL) ? reinterpret_cast<const WCHAR *>(text->get_utf16(first, last)) : NULL;
                if (pText == NULL)
                    return;
                size_t length = text->range_length(first, last);

                // Obtain the font family
                LSPString family_name;
                WinDisplay::font_t *custom = NULL;
                IDWriteFontFamily *ff   = pDisplay->get_font_family(f, &family_name, &custom);
                lsp_finally{ safe_release(ff); };


                if (custom != NULL)
                {
                    if (try_out_text_relative(custom->collection, custom->family, custom->wname, f, color, x, y, dx, dy, pText, length))
                        return;
                }
                if (ff != NULL)
                {
                    if (try_out_text_relative(NULL, ff, family_name.get_utf16(), f, color, x, y, dx, dy, pText, length))
                        return;
                }
            }

            bool WinDDSurface::get_antialiasing()
            {
                if (pDC == NULL)
                    return false;
                return pDC->GetAntialiasMode() != D2D1_ANTIALIAS_MODE_ALIASED;
            }

            bool WinDDSurface::set_antialiasing(bool set)
            {
                if (pDC == NULL)
                    return false;
                bool old    = pDC->GetAntialiasMode() != D2D1_ANTIALIAS_MODE_ALIASED;
                pDC->SetAntialiasMode((set) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
                return old;
            }

            surf_line_cap_t WinDDSurface::get_line_cap()
            {
                return SURFLCAP_BUTT;
            }

            surf_line_cap_t WinDDSurface::set_line_cap(surf_line_cap_t lc)
            {
                return SURFLCAP_BUTT;
            }
        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

