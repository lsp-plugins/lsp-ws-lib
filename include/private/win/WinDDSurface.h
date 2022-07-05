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

#include <d2d1.h>
#include <windows.h>


namespace lsp
{
    namespace ws
    {
        namespace win
        {
            class WinDDSurface: public ISurface
            {
                protected:
                    WinDisplay             *pDisplay;
                    HWND                    hWindow;
                    ID2D1RenderTarget      *pDC;

                public:
                    explicit WinDDSurface(WinDisplay *dpy, HWND hwnd, size_t width, size_t height);
                    explicit WinDDSurface(WinDisplay *dpy, size_t width, size_t height);
                    virtual ~WinDDSurface();

                    virtual void destroy() override;

                public:
                    virtual ISurface *create(size_t width, size_t height) override;
                    virtual ISurface *create_copy() override;

                    virtual IGradient *linear_gradient(float x0, float y0, float x1, float y1) override;
                    virtual IGradient *radial_gradient
                    (
                        float cx0, float cy0, float r0,
                        float cx1, float cy1, float r1
                    ) override;

                    virtual void begin() override;
                    virtual void end() override;

                public:
                    virtual void draw(ISurface *s, float x, float y) override;
                    virtual void draw(ISurface *s, float x, float y, float sx, float sy) override;
                    virtual void draw(ISurface *s, const ws::rectangle_t *r) override;
                    virtual void draw_alpha(ISurface *s, float x, float y, float sx, float sy, float a) override;
                    virtual void draw_rotate_alpha(ISurface *s, float x, float y, float sx, float sy, float ra, float a) override;
                    virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh) override;

                    virtual void fill_rect(const Color &color, float left, float top, float width, float height) override;
                    virtual void fill_rect(const Color &color, const ws::rectangle_t *r) override;
                    virtual void fill_rect(IGradient *g, float left, float top, float width, float height) override;
                    virtual void fill_rect(IGradient *g, const ws::rectangle_t *r) override;

                    virtual void wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width) override;

                    virtual void fill_round_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_round_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r) override;
                    virtual void fill_round_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_round_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r) override;

                    virtual void full_rect(float left, float top, float width, float height, float line_width, const Color &color) override;

                    virtual void fill_sector(float cx, float cy, float radius, float angle1, float angle2, const Color &color) override;

                    virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, IGradient *g) override;

                    virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const Color &color) override;

                    virtual bool get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const char *text) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual void clear(const Color &color) override;
                    virtual void clear_rgb(uint32_t color) override;
                    virtual void clear_rgba(uint32_t color) override;

                    virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual void square_dot(float x, float y, float width, const Color &color) override;
                    virtual void square_dot(float x, float y, float width, float r, float g, float b, float a) override;

                    virtual void line(float x0, float y0, float x1, float y1, float width, const Color &color) override;
                    virtual void line(float x0, float y0, float x1, float y1, float width, IGradient *g) override;

                    virtual void parametric_line(float a, float b, float c, float width, const Color &color) override;
                    virtual void parametric_line(float a, float b, float c, float left, float right, float top, float bottom, float width, const Color &color) override;

                    virtual void parametric_bar(float a1, float b1, float c1, float a2, float b2, float c2,
                            float left, float right, float top, float bottom, IGradient *gr) override;

                    virtual void wire_arc(float x, float y, float r, float a1, float a2, float width, const Color &color) override;

                    virtual void fill_frame(const Color &color,
                            float fx, float fy, float fw, float fh,
                            float ix, float iy, float iw, float ih
                            ) override;

                    virtual void fill_frame(const Color &color, const ws::rectangle_t *out, const ws::rectangle_t *in) override;

                    virtual void fill_round_frame(
                            const Color &color,
                            float radius, size_t flags,
                            float fx, float fy, float fw, float fh,
                            float ix, float iy, float iw, float ih
                        ) override;

                    virtual void fill_round_frame(
                            const Color &color, float radius, size_t flags,
                            const ws::rectangle_t *out, const ws::rectangle_t *in
                        ) override;

                    virtual void fill_poly(const Color & color, const float *x, const float *y, size_t n) override;
                    virtual void fill_poly(IGradient *gr, const float *x, const float *y, size_t n) override;
                    virtual void wire_poly(const Color & color, float width, const float *x, const float *y, size_t n) override;
                    virtual void draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n) override;
                    virtual void fill_circle(float x, float y, float r, const Color & color) override;
                    virtual void fill_circle(float x, float y, float r, IGradient *g) override;

                    virtual void clip_begin(float x, float y, float w, float h) override;
                    virtual void clip_begin(const ws::rectangle_t *area) override;

                    virtual void clip_end() override;

                    virtual bool get_antialiasing() override;
                    virtual bool set_antialiasing(bool set) override;

                    virtual surf_line_cap_t get_line_cap() override;
                    virtual surf_line_cap_t set_line_cap(surf_line_cap_t lc) override;

                    virtual     void *start_direct() override;

                    virtual     void end_direct() override;

                public:
                    void        sync_size();
            };

        } /* namespace win */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PLATFORM_WINDOWS */

#endif /* PRIVATE_WIN_WINDDSURFACE_H_ */
