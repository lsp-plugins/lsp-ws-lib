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

#ifndef PRIVATE_WIN_WINDDSURFACE_H_
#define PRIVATE_WIN_WINDDSURFACE_H_

#include <lsp-plug.in/common/types.h>

#ifdef PLATFORM_WINDOWS

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/ISurface.h>

#include <private/win/WinDisplay.h>
#include <private/win/com.h>

#include <d2d1.h>
#include <windows.h>


namespace lsp
{
    namespace ws
    {
        namespace win
        {
            // The shared context between surfaces. Controls the validity of the root drawing context
            // by introducing the number of version of this context.
            typedef struct LSP_HIDDEN_MODIFIER WinDDShared
            {
                size_t              nReferences;    // References to this shared object
                size_t              nVersion;       // The version number of root drawing context
                WinDisplay         *pDisplay;       // The pointer to the display
                HWND                hWindow;        // The pointer to the window, owner of context

                WinDDShared(WinDisplay *dpy, HWND wnd);
                ~WinDDShared();

                size_t              AddRef();
                size_t              Release();
                void                Invalidate();
            } WinDDShared;

            class LSP_HIDDEN_MODIFIER WinDDSurface: public ISurface
            {
                protected:
                    WinDDShared                *pShared;        // Pointer to the shared drawing resource
                    size_t                      nVersion;       // The version, for valid surface should match the shared version
                    ID2D1RenderTarget          *pDC;            // Pointer to drawing context

                #ifdef LSP_DEBUG
                    ssize_t                     nClipping;
                #endif /* LSP_DEBUG */

                public:
                    explicit WinDDSurface(WinDisplay *dpy, HWND hwnd, size_t width, size_t height);
                    explicit WinDDSurface(WinDDShared *shared, ID2D1RenderTarget *dc, size_t width, size_t height);
                    virtual ~WinDDSurface();

                    virtual void destroy() override;

                    virtual bool valid() const override;

                protected:
                    inline bool bad_state() const;
                    void    do_destroy();

                    void    draw_rounded_rectangle(const D2D_RECT_F &rect, size_t mask, float radius, float line_width, ID2D1Brush *brush);
                    void    draw_triangle(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2);
                    void    draw_negative_arc(ID2D1Brush *brush, float x0, float y0, float x1, float y1, float x2, float y2);
                    void    draw_polygon(ID2D1Brush *brush, const float *x, const float *y, size_t n, float width);

                    bool    try_out_text(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, const WCHAR *text, size_t length);
                    bool    try_out_text_relative(IDWriteFontCollection *fc, IDWriteFontFamily *ff, const WCHAR *family, const Font &f, const Color &color, float x, float y, float dx, float dy, const WCHAR *text, size_t length);

                public:
                    virtual IDisplay *display() override;

                    virtual ISurface *create(size_t width, size_t height) override;
                    virtual ISurface *create_copy() override;

                    virtual IGradient *linear_gradient(float x0, float y0, float x1, float y1) override;
                    virtual IGradient *radial_gradient
                    (
                        float cx0, float cy0,
                        float cx1, float cy1,
                        float r
                    ) override;

                    virtual void begin() override;
                    virtual void end() override;

                public:
                    virtual void clear(const Color &color) override;
                    virtual void clear_rgb(uint32_t color) override;
                    virtual void clear_rgba(uint32_t color) override;

                    virtual void wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width) override;

                    virtual void fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r) override;
                    virtual void fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r) override;

                    virtual void fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2) override;
                    virtual void fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2) override;
                    virtual void fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2) override;
                    virtual void fill_circle(const Color & c, float x, float y, float r) override;
                    virtual void fill_circle(IGradient *g, float x, float y, float r) override;
                    virtual void wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width) override;

                    virtual void line(const Color &c, float x0, float y0, float x1, float y1, float width) override;
                    virtual void line(IGradient *g, float x0, float y0, float x1, float y1, float width) override;

                    virtual void parametric_line(const Color &color, float a, float b, float c, float width) override;
                    virtual void parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width) override;

                    virtual void parametric_bar(
                        IGradient *g,
                        float a1, float b1, float c1, float a2, float b2, float c2,
                        float left, float right, float top, float bottom) override;

                    virtual void fill_poly(const Color &c, const float *x, const float *y, size_t n) override;
                    virtual void fill_poly(IGradient *gr, const float *x, const float *y, size_t n) override;
                    virtual void wire_poly(const Color &c, float width, const float *x, const float *y, size_t n) override;
                    virtual void draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n) override;

                    virtual void fill_frame(
                        const Color &color,
                        size_t flags, float radius,
                        float fx, float fy, float fw, float fh,
                        float ix, float iy, float iw, float ih) override;

                    // Font and text parameter estimation
                    virtual bool get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual void draw(ISurface *s, float x, float y, float sx, float sy, float a) override;
                    virtual void draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a) override;
                    virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a) override;
                    virtual void draw_raw(
                        const void *data, size_t width, size_t height, size_t stride,
                        float x, float y, float sx, float sy, float a) override;

                    virtual void clip_begin(float x, float y, float w, float h) override;
                    virtual void clip_end() override;

                    virtual bool get_antialiasing() override;
                    virtual bool set_antialiasing(bool set) override;

                    virtual surf_line_cap_t get_line_cap() override;
                    virtual surf_line_cap_t set_line_cap(surf_line_cap_t lc) override;

                public:
                    void        sync_size();
            };

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINDDSURFACE_H_ */
