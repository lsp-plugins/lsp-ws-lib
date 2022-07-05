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
                {
                    pDC->Release();
                    pDC     = NULL;
                }
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
                return NULL;
            }

            IGradient *WinDDSurface::radial_gradient
            (
                float cx0, float cy0, float r0,
                float cx1, float cy1, float r1
            )
            {
                return NULL;
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

            void WinDDSurface::fill_rect(const Color &color, float left, float top, float width, float height)
            {
            }

            void WinDDSurface::fill_rect(const Color &color, const ws::rectangle_t *r)
            {
            }

            void WinDDSurface::fill_rect(IGradient *g, float left, float top, float width, float height)
            {
            }

            void WinDDSurface::fill_rect(IGradient *g, const ws::rectangle_t *r)
            {
            }

            void WinDDSurface::wire_rect(const Color &color, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_rect(IGradient *g, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect_inside(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect_inside(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect_inside(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
            }

            void WinDDSurface::wire_round_rect_inside(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width)
            {
            }

            void WinDDSurface::fill_round_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
            {
            }

            void WinDDSurface::fill_round_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
            {
            }

            void WinDDSurface::fill_round_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
            }

            void WinDDSurface::fill_round_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
            }

            void WinDDSurface::full_rect(float left, float top, float width, float height, float line_width, const Color &color)
            {
            }

            void WinDDSurface::fill_sector(float cx, float cy, float radius, float angle1, float angle2, const Color &color)
            {
            }

            void WinDDSurface::fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, IGradient *g)
            {
            }

            void WinDDSurface::fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const Color &color)
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
            }

            void WinDDSurface::clear_rgb(uint32_t color)
            {
            }

            void WinDDSurface::clear_rgba(uint32_t color)
            {
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

            void WinDDSurface::square_dot(float x, float y, float width, const Color &color)
            {
            }

            void WinDDSurface::square_dot(float x, float y, float width, float r, float g, float b, float a)
            {
            }

            void WinDDSurface::line(float x0, float y0, float x1, float y1, float width, const Color &color)
            {
            }

            void WinDDSurface::line(float x0, float y0, float x1, float y1, float width, IGradient *g)
            {
            }

            void WinDDSurface::parametric_line(float a, float b, float c, float width, const Color &color)
            {
            }

            void WinDDSurface::parametric_line(float a, float b, float c, float left, float right, float top, float bottom, float width, const Color &color)
            {
            }

            void WinDDSurface::parametric_bar(float a1, float b1, float c1, float a2, float b2, float c2,
                    float left, float right, float top, float bottom, IGradient *gr)
            {
            }

            void WinDDSurface::wire_arc(float x, float y, float r, float a1, float a2, float width, const Color &color)
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

            void WinDDSurface::fill_circle(float x, float y, float r, const Color & color)
            {
            }

            void WinDDSurface::fill_circle(float x, float y, float r, IGradient *g)
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

