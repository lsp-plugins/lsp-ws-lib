/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <private/win/com.h>

#include <d2d1.h>
#include <wincodec.h>
#include <windows.h>

namespace lsp
{
    namespace ws
    {
        namespace win
        {
            constexpr FLOAT DEFAULT_DESKTOP_DPI     = 96.0f;

            //-----------------------------------------------------------------
            WinDDShared::WinDDShared(WinDisplay *dpy, HWND wnd)
            {
                nReferences     = 0;
                nVersion        = 0;
                pDisplay        = dpy;
                hWindow         = wnd;
            }

            WinDDShared::~WinDDShared()
            {
            }

            size_t WinDDShared::AddRef()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            size_t WinDDShared::Release()
            {
                size_t count = atomic_add(&nReferences, -1) - 1;
                if (count == 0)
                    delete this;
                return count;
            }

            void WinDDShared::Invalidate()
            {
                ++nVersion;
            }

            //-----------------------------------------------------------------
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

            WinDDSurface::WinDDSurface(WinDisplay *dpy, HWND hwnd, size_t width, size_t height):
                ISurface(width, height, ST_DDRAW)
            {
                // Create initial shared context
                pShared             = safe_acquire(new WinDDShared(dpy, hwnd));
                nVersion            = pShared->nVersion;
                pDC                 = NULL;
                pStrokeStyle        = NULL;

            #ifdef LSP_DEBUG
                nClipping           = 0;
            #endif /* LSP_DEBUG */
            }

            WinDDSurface::WinDDSurface(WinDDShared *shared, ID2D1RenderTarget *dc, size_t width, size_t height):
                ISurface(width, height, ST_IMAGE)
            {
                pShared             = safe_acquire(shared);
                nVersion            = pShared->nVersion;
                pDC                 = dc;
                pStrokeStyle        = NULL;

            #ifdef LSP_DEBUG
                nClipping           = 0;
            #endif /* LSP_DEBUG */
            }

            WinDDSurface::~WinDDSurface()
            {
                do_destroy();
            }

            void WinDDSurface::destroy()
            {
                do_destroy();
            }

            bool WinDDSurface::valid() const
            {
                return (pShared != NULL) && (nVersion == pShared->nVersion);
            }

            bool WinDDSurface::bad_state() const
            {
                return (pShared == NULL) || (pDC == NULL);
            }

            void WinDDSurface::do_destroy()
            {
                // Release stroke style
                safe_release(pStrokeStyle);

                // Do some manipulations with data references
                if (pShared != NULL)
                {
                    if (nType == ST_DDRAW)
                        pShared->Invalidate();
                    safe_release(pShared);
                }

                // Release drawing context
                safe_release(pDC);
            }

            void WinDDSurface::begin()
            {
                if (pShared == NULL)
                    return;

                // Release drawing context if the shared version has changed
                if (nVersion != pShared->nVersion)
                    safe_release(pDC);

                // Create render target if necessary
                if ((pDC == NULL) && (nType == ST_DDRAW))
                {
                    D2D1_RENDER_TARGET_PROPERTIES prop;
                    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProp;

                    prop.type                   = D2D1_RENDER_TARGET_TYPE_DEFAULT;
                    prop.pixelFormat.format     = DXGI_FORMAT_B8G8R8A8_UNORM;
                    prop.pixelFormat.alphaMode  = D2D1_ALPHA_MODE_PREMULTIPLIED; // D2D1_ALPHA_MODE_STRAIGHT;
                    prop.dpiX                   = DEFAULT_DESKTOP_DPI;
                    prop.dpiY                   = DEFAULT_DESKTOP_DPI;
                    prop.usage                  = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
                    prop.minLevel               = D2D1_FEATURE_LEVEL_DEFAULT;

                    hwndProp.hwnd               = pShared->hWindow;
                    hwndProp.pixelSize.width    = nWidth;
                    hwndProp.pixelSize.height   = nHeight;
                    hwndProp.presentOptions     = D2D1_PRESENT_OPTIONS_NONE;

                    ID2D1HwndRenderTarget *ht   = NULL;
                    HRESULT hr                  = pShared->pDisplay->d2d_factory()->CreateHwndRenderTarget(prop, hwndProp, &ht);
                    if (FAILED(hr))
                    {
                        lsp_error("Error creating HWND render target: window=%p, error=0x%08lx", pShared->hWindow, long(hr));
                        return;
                    }

                    pDC                         = ht;
                    nVersion                    = ++pShared->nVersion;
                }

                if (pDC != NULL)
                {
                    pDC->BeginDraw();
                    pDC->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
            }

            void WinDDSurface::end()
            {
                if (bad_state())
                    return;

            #ifdef LSP_DEBUG
                if (nClipping > 0)
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
            #endif /* LSP_DEBUG */

                HRESULT hr = pDC->EndDraw();
                if (FAILED(hr) || (hr == HRESULT(D2DERR_RECREATE_TARGET)))
                {
                    // Invalidate shared context and release device context
                    pShared->Invalidate();
                    safe_release(pDC);
                }
            }

            void WinDDSurface::invalidate()
            {
                if (pShared != NULL)
                    pShared->Invalidate();
                safe_release(pDC);
            }

            status_t WinDDSurface::resize(size_t width, size_t height)
            {
                if ((pShared == NULL) || (pShared->hWindow == NULL))
                    return STATUS_BAD_STATE;

                nWidth      = width;
                nHeight     = height;

                if (pDC != NULL)
                {
                    ID2D1HwndRenderTarget *ht   = static_cast<ID2D1HwndRenderTarget *>(pDC);

                    D2D1_SIZE_U size;
                    size.width      = width;
                    size.height     = height;
                    ht->Resize(&size);
                }

                return STATUS_OK;
            }

            void WinDDSurface::clear(const Color &color)
            {
                if (bad_state())
                    return;

                D2D_COLOR_F c;
                color.get_rgbo(c.r, c.g, c.b, c.a);
                pDC->Clear(&c);
            }

            void WinDDSurface::clear_rgb(uint32_t color)
            {
                if (bad_state())
                    return;

                D2D_COLOR_F c;
                c.r     = ((color >> 16) & 0xff) / 255.0f;
                c.g     = ((color >> 8) & 0xff) / 255.0f;
                c.b     = (color & 0xff) / 255.0f;
                c.a     = 1.0f;
                pDC->Clear(&c);
            }

            void WinDDSurface::clear_rgba(uint32_t color)
            {
                if (bad_state())
                    return;

                D2D_COLOR_F c;
                c.r     = ((color >> 16) & 0xff) / 255.0f;
                c.g     = ((color >> 8) & 0xff) / 255.0f;
                c.b     = (color & 0xff) / 255.0f;
                c.a     = 1.0f - ((color >> 24) & 0xff) / 255.0f;
                pDC->Clear(&c);
            }

            IGradient *WinDDSurface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                if (bad_state())
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
                if (bad_state())
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
                    return;
                }

                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                if (bad_state())
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
                if (bad_state())
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
                if (bad_state())
                    return;
                if (g == NULL)
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
                if (bad_state())
                    return;
                if (g == NULL)
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
                if (bad_state())
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
                if (bad_state())
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
                if (bad_state())
                    return;
                if (g == NULL)
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
                if (bad_state())
                    return;
                if (g == NULL)
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

            void WinDDSurface::fill_sector(const Color &c, float x, float y, float r, float a1, float a2)
            {
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                // Check if we need just to draw circle
                if (fabs(a2 - a1) >= M_PI * 2.0f)
                {
                    pDC->FillEllipse(
                        D2D1::Ellipse(D2D1::Point2F(x, y), r, r),
                        brush);
                    return;
                }

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                    D2D1::Point2F(x, y),
                    D2D1_FIGURE_BEGIN_FILLED);
                s->AddLine(D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)));
                if (a2 < a1)
                {
                    // Draw arc with 90-degree segments
                    do
                    {
                        a1  = lsp_max(a1 - M_PI * 0.5f, a2);
                        s->AddArc(
                            D2D1::ArcSegment(
                                D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)),
                                D2D1::SizeF(r, r),
                                0.0f,
                                D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                                D2D1_ARC_SIZE_SMALL));
                    } while (a2 < a1);
                }
                else
                {
                    // Draw arc with 90-degree segments
                    do
                    {
                        a1  = lsp_min(a1 + M_PI * 0.5f, a2);
                        s->AddArc(
                            D2D1::ArcSegment(
                                D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)),
                                D2D1::SizeF(r, r),
                                0.0f,
                                D2D1_SWEEP_DIRECTION_CLOCKWISE,
                                D2D1_ARC_SIZE_SMALL));
                    } while (a2 > a1);
                }
                s->EndFigure(D2D1_FIGURE_END_CLOSED);
                s->Close();

                // Draw the geometry
                pDC->FillGeometry(g, brush, NULL);
            }

            void WinDDSurface::draw_triangle(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                draw_triangle(brush, x0, y0, x1, y1, x2, y2);
            }

            void WinDDSurface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                if (bad_state())
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;

                draw_triangle(brush, x0, y0, x1, y1, x2, y2);
            }

            void WinDDSurface::fill_circle(const Color & c, float x, float y, float r)
            {
                if (bad_state())
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
                if (bad_state())
                    return;
                if (g == NULL)
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
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                // Check if we need just to draw circle
                if (fabs(a2 - a1) >= M_PI * 2.0f)
                {
                    pDC->DrawEllipse(
                        D2D1::Ellipse(D2D1::Point2F(x, y), r, r),
                        brush,
                        width,
                        pStrokeStyle);
                    return;
                }

                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                if (a2 < a1)
                {
                    // Draw arc with 90-degree segments
                    do
                    {
                        a1  = lsp_max(a1 - M_PI * 0.5f, a2);
                        s->AddArc(
                            D2D1::ArcSegment(
                                D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)),
                                D2D1::SizeF(r, r),
                                0.0f,
                                D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                                D2D1_ARC_SIZE_SMALL));
                    } while (a2 < a1);
                }
                else
                {
                    // Draw arc with 90-degree segments
                    do
                    {
                        a1  = lsp_min(a1 + M_PI * 0.5f, a2);
                        s->AddArc(
                            D2D1::ArcSegment(
                                D2D1::Point2F(x + r * cosf(a1), y + r * sinf(a1)),
                                D2D1::SizeF(r, r),
                                0.0f,
                                D2D1_SWEEP_DIRECTION_CLOCKWISE,
                                D2D1_ARC_SIZE_SMALL));
                    } while (a2 > a1);
                }
                s->EndFigure(D2D1_FIGURE_END_OPEN);
                s->Close();

                // Draw the geometry
                pDC->DrawGeometry(g, brush, width, pStrokeStyle);
            }

            void WinDDSurface::line(const Color &c, float x0, float y0, float x1, float y1, float width)
            {
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(c), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                pDC->DrawLine(
                    D2D1::Point2F(x0, y0),
                    D2D1::Point2F(x1, y1),
                    brush, width, pStrokeStyle);
            }

            void WinDDSurface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                if (bad_state())
                    return;
                if (g == NULL)
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(g)->get_brush();
                if (brush == NULL)
                    return;
                pDC->DrawLine(
                    D2D1::Point2F(x0, y0),
                    D2D1::Point2F(x1, y1),
                    brush, width, pStrokeStyle);
            }

            void WinDDSurface::parametric_line(const Color &color, float a, float b, float c, float width)
            {
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                if (fabs(a) > fabs(b))
                    pDC->DrawLine(
                        D2D1::Point2F(- c / a, 0.0f),
                        D2D1::Point2F(-(c + b*nHeight)/a, nHeight),
                        brush, width, pStrokeStyle);
                else
                    pDC->DrawLine(
                        D2D1::Point2F(0.0f, - c / b),
                        D2D1::Point2F(nWidth, -(c + a*nWidth)/b),
                        brush, width, pStrokeStyle);
            }

            void WinDDSurface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                if (bad_state())
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                if (fabs(a) > fabs(b))
                    pDC->DrawLine(
                        D2D1::Point2F(roundf(-(c + b*top)/a), roundf(top)),
                        D2D1::Point2F(roundf(-(c + b*bottom)/a), roundf(bottom)),
                        brush, width, pStrokeStyle);
                else
                    pDC->DrawLine(
                        D2D1::Point2F(roundf(left), roundf(-(c + a*left)/b)),
                        D2D1::Point2F(roundf(right), roundf(-(c + a*right)/b)),
                        brush, width, pStrokeStyle);
            }

            void WinDDSurface::parametric_bar(
                IGradient *gr,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                if (bad_state())
                    return;
                if (gr == NULL)
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(gr)->get_brush();
                if (brush == NULL)
                    return;

                // Create geometry object
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                    pDC->FillGeometry(g, brush, NULL);
                else
                    pDC->DrawGeometry(g, brush, width, pStrokeStyle);
            }

            void WinDDSurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
            {
                if (bad_state())
                    return;
                if (n < 2)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                draw_polygon(brush, x, y, n, -1.0f);
            }

            void WinDDSurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
            {
                if (bad_state())
                    return;
                if ((gr == NULL) || (n < 2))
                    return;

                ID2D1Brush *brush   = static_cast<WinDDGradient *>(gr)->get_brush();
                if (brush == NULL)
                    return;

                draw_polygon(brush, x, y, n, -1.0f);
            }

            void WinDDSurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
            {
                if (bad_state())
                    return;
                if (n < 2)
                    return;

                ID2D1SolidColorBrush *brush = NULL;
                if (FAILED(pDC->CreateSolidColorBrush(d2d_color(color), &brush)))
                    return;
                lsp_finally{ safe_release(brush); };

                draw_polygon(brush, x, y, n, width);
            }

            void WinDDSurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                if (bad_state())
                    return;
                if (n < 2)
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
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                pDC->FillGeometry(g, f_brush, NULL);
                pDC->DrawGeometry(g, w_brush, width, pStrokeStyle);
            }

            void WinDDSurface::draw_negative_arc(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                // Create geometry object for the sector
                ID2D1PathGeometry *g = NULL;
                if (FAILED(pShared->pDisplay->d2d_factory()->CreatePathGeometry(&g)))
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
                if (bad_state())
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
                if (bad_state())
                    return;

                pDC->PushAxisAlignedClip(
                    D2D1::RectF(x, y, x+w, y+h),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

                #ifdef LSP_DEBUG
                ++ nClipping;
                #endif /* LSP_DEBUG */
            }

            void WinDDSurface::clip_end()
            {
                if (bad_state())
                    return;

            #ifdef LSP_DEBUG
                if (nClipping <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                -- nClipping;
            #endif /* LSP_DEBUG */

                pDC->PopAxisAlignedClip();
            }

            IDisplay *WinDDSurface::display()
            {
                return (pShared != NULL) ? pShared->pDisplay : NULL;
            }

            ISurface *WinDDSurface::create(size_t width, size_t height)
            {
                if (bad_state())
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
                    D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, // options
                    &dc);
                if (FAILED(hr))
                    return NULL;

                return new WinDDSurface(pShared, dc, width, height);
            }

            void WinDDSurface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                if (bad_state())
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
                if (bad_state())
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
                if (bad_state())
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
                if (bad_state())
                    return;

                HRESULT hr;

                // Create WIC bitmap
                IWICBitmap *wic = NULL;
                hr = pShared->pDisplay->wic_factory()->CreateBitmapFromMemory(
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
                return (pShared != NULL) ? pShared->pDisplay->get_font_parameters(f, fp) : false;
            }

            bool WinDDSurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
            {
                // Redirect call to the display
                return (pShared != NULL) ? pShared->pDisplay->get_text_parameters(f, tp, text, first, last) : false;
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if (bad_state())
                    return;
                if (text == NULL)
                    return;

                LSPString tmp;
                if (tmp.set_utf8(text))
                    out_text(f, color, x, y, &tmp, 0, tmp.length());
            }

            bool WinDDSurface::try_out_text(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, const lsp_wchar_t *text, size_t length)
            {
                // Get font face
                IDWriteFontFace *face   = pShared->pDisplay->get_font_face(f, ff);
                if (face == NULL)
                    return false;
                lsp_finally { safe_release(face); };

                // Obtain font metrics
                DWRITE_FONT_METRICS fm;
                face->GetMetrics(&fm);

                // Create glyph run
                win::glyph_run_t *run   = pShared->pDisplay->make_glyph_run(f, face, &fm, text, length);
                if (run == NULL)
                    return false;
                lsp_finally { free(run); };

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

                pDC->DrawGlyphRun(
                    D2D1_POINT_2F{x, y},
                    run->run,
                    brush,
                    DWRITE_MEASURING_MODE_NATURAL);

                if (f.is_underline())
                {
                    ws::text_parameters_t tp;
                    calc_text_metrics(f, &tp, &fm, run->metrics, length);

                    float scale = f.size() / float(fm.designUnitsPerEm);
                    float k     = f.bold() ? 0.6f : 0.5f;
                    float ypos  = y - fm.underlinePosition * scale;
                    float thick = k * fm.underlineThickness * scale;

                    pDC->FillRectangle(
                        D2D1_RECT_F {
                            x,
                            ypos - thick,
                            x + tp.Width,
                            ypos + thick
                        },
                        brush);
                }

                pDC->SetTextAntialiasMode(antialias);

                return true;
            }

            void WinDDSurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (bad_state())
                    return;
                if ((first < 0) || (last < 0) || (first >= last))
                    return;

                size_t range    = text->range_length(first, last);
                const lsp_wchar_t *data = text->characters();

                // Obtain the font family
                LSPString family_name;
                WinDisplay::font_t *custom = NULL;
                IDWriteFontFamily *ff   = pShared->pDisplay->get_font_family(f, &family_name, &custom);
                lsp_finally{ safe_release(ff); };

                if (custom != NULL)
                {
                    if (try_out_text(custom->collection, custom->family, custom->wname, f, color, x, y, &data[first], range))
                        return;
                }
                if (ff != NULL)
                {
                    if (try_out_text(NULL, ff, family_name.get_utf16(), f, color, x, y, &data[first], range))
                        return;
                }
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if (bad_state())
                    return;
                if (text == NULL)
                    return;

                LSPString tmp;
                if (!tmp.set_utf8(text))
                    return;

                return out_text_relative(f, color, x, y, dx, dy, &tmp, 0, tmp.length());
            }

            bool WinDDSurface::try_out_text_relative(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, float dx, float dy, const lsp_wchar_t *text, size_t length)
            {
                IDWriteFontFace *face   = pShared->pDisplay->get_font_face(f, ff);
                if (face == NULL)
                    return false;
                lsp_finally { safe_release(face); };

                // Obtain font metrics
                DWRITE_FONT_METRICS fm;
                face->GetMetrics(&fm);

                // Create glyph run
                win::glyph_run_t *run   = pShared->pDisplay->make_glyph_run(f, face, &fm, text, length);
                if (run == NULL)
                    return false;
                lsp_finally { free(run); };

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

                ws::text_parameters_t tp;
                calc_text_metrics(f, &tp, &fm, run->metrics, length);

                float r_w       = tp.Width;
                float r_h       = tp.Height;
                float fx        = x - tp.XBearing - r_w * 0.5f + (r_w + 4.0f) * 0.5f * dx;
                float fy        = y + r_h * 0.5f - (r_h + 4.0f) * 0.5f * dy;

                pDC->DrawGlyphRun(
                    D2D1_POINT_2F{fx, fy},
                    run->run,
                    brush,
                    DWRITE_MEASURING_MODE_NATURAL);

                if (f.is_underline())
                {
                    float scale = f.size() / float(fm.designUnitsPerEm);
                    float k     = f.bold() ? 0.6f : 0.5f;
                    float ypos  = fy - fm.underlinePosition * scale;
                    float thick = k * fm.underlineThickness * scale;

                    pDC->FillRectangle(
                        D2D1_RECT_F {
                            fx,
                            ypos - thick,
                            fx + tp.Width,
                            ypos + thick
                        },
                        brush);
                }

                pDC->SetTextAntialiasMode(antialias);

                return true;
            }

            void WinDDSurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if (bad_state())
                    return;
                if ((first < 0) || (last < 0) || (first >= last))
                    return;

                size_t range    = text->range_length(first, last);
                const lsp_wchar_t *data = text->characters();

                // Obtain the font family
                LSPString family_name;
                WinDisplay::font_t *custom = NULL;
                IDWriteFontFamily *ff   = pShared->pDisplay->get_font_family(f, &family_name, &custom);
                lsp_finally{ safe_release(ff); };


                if (custom != NULL)
                {
                    if (try_out_text_relative(custom->collection, custom->family, custom->wname, f, color, x, y, dx, dy, &data[first], range))
                        return;
                }
                if (ff != NULL)
                {
                    if (try_out_text_relative(NULL, ff, family_name.get_utf16(), f, color, x, y, dx, dy, &data[first], range))
                        return;
                }
            }

            bool WinDDSurface::get_antialiasing()
            {
                if (bad_state())
                    return false;
                return pDC->GetAntialiasMode() != D2D1_ANTIALIAS_MODE_ALIASED;
            }

            bool WinDDSurface::set_antialiasing(bool set)
            {
                if (bad_state())
                    return false;
                bool old    = pDC->GetAntialiasMode() != D2D1_ANTIALIAS_MODE_ALIASED;
                pDC->SetAntialiasMode((set) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
                return old;
            }

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

