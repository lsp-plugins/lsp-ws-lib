/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 25 окт. 2016 г.
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

#include <lsp-plug.in/ws/IDisplay.h>
#include <lsp-plug.in/ws/ISurface.h>
#include <stdlib.h>

namespace lsp
{
    namespace ws
    {
        ISurface::ISurface(size_t width, size_t height, surface_type_t type)
        {
            nWidth      = width;
            nHeight     = height;
            nType       = type;
        }

        ISurface::ISurface()
        {
            nWidth      = 0;
            nHeight     = 0;
            nType       = ST_UNKNOWN;
        }

        ISurface::~ISurface()
        {
            nWidth      = 0;
            nHeight     = 0;
            nType       = ST_UNKNOWN;
        }

        IDisplay *ISurface::display()
        {
            return NULL;
        }

        ISurface *ISurface::create(size_t width, size_t height)
        {
            return new ISurface(width, height, ST_UNKNOWN);
        }

        status_t ISurface::resize(size_t width, size_t height)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        void ISurface::destroy()
        {
        }

        IGradient *ISurface::linear_gradient(float x0, float y0, float x1, float y1)
        {
            return new IGradient();
        }

        IGradient *ISurface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
        {
            return new IGradient();
        }

        void ISurface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
        {
        }

        void ISurface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
        {
        }

        void ISurface::draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a)
        {
        }

        void ISurface::draw_raw(
            const void *data, size_t width, size_t height, size_t stride,
            float x, float y, float sx, float sy, float a)
        {
        }

        void ISurface::wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
        {
        }

        void ISurface::wire_rect(const Color &c, size_t mask, float radius, const rectangle_t *rect, float line_width)
        {
            wire_rect(c, mask, radius, rect->nLeft, rect->nTop, rect->nWidth, rect->nHeight, line_width);
        }

        void ISurface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
        {
        }

        void ISurface::wire_rect(IGradient *g, size_t mask, float radius, const rectangle_t *rect, float line_width)
        {
            wire_rect(g, mask, radius, rect->nLeft, rect->nTop, rect->nWidth, rect->nHeight, line_width);
        }

        void ISurface::fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height)
        {
        }

        void ISurface::fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r)
        {
            fill_rect(color, radius, mask, r->nLeft, r->nTop, r->nWidth, r->nHeight);
        }

        void ISurface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
        {
        }

        void ISurface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
        {
            fill_rect(g, radius, mask, r->nLeft, r->nTop, r->nWidth, r->nHeight);
        }

        void ISurface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, float left, float top, float width, float height)
        {
        }

        void ISurface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, const ws::rectangle_t *r)
        {
            fill_rect(s, alpha, mask, radius, r->nLeft, r->nTop, r->nWidth, r->nHeight);
        }

        void ISurface::fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2)
        {
        }

        void ISurface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
        {
        }

        void ISurface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
        {
        }

        bool ISurface::get_font_parameters(const Font &f, font_parameters_t *fp)
        {
            return false;
        }

        bool ISurface::get_text_parameters(const Font &f, text_parameters_t *tp, const char *text)
        {
            if (text == NULL)
                return false;
            LSPString tmp;
            if (!tmp.set_utf8(text))
                return false;
            return get_text_parameters(f, tp, &tmp, 0, tmp.length());
        }

        bool ISurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text)
        {
            if (text == NULL)
                return false;
            return get_text_parameters(f, tp, text, 0, text->length());
        }

        bool ISurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first)
        {
            if (text == NULL)
                return false;
            return get_text_parameters(f, tp, text, first, text->length());
        }

        bool ISurface::get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last)
        {
            return false;
        }

        void ISurface::clear(const Color &color)
        {
        }

        void ISurface::begin()
        {
        }

        void ISurface::end()
        {
        }

        bool ISurface::valid() const
        {
            return true;
        }

        void ISurface::clear_rgb(uint32_t color)
        {
        }

        void ISurface::clear_rgba(uint32_t color)
        {
        }

        void ISurface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
        {
        }

        void ISurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text)
        {
            if (text == NULL)
                return;
            out_text(f, color, x, y, text, 0, text->length());
        }

        void ISurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first)
        {
            if (text == NULL)
                return;
            out_text(f, color, x, y, text, first, text->length());
        }

        void ISurface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
        {
        }

        void ISurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
        {
        }

        void ISurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text)
        {
            if (text == NULL)
                return;
            out_text_relative(f, color, x, y, dx, dy, text, 0, text->length());
        }

        void ISurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first)
        {
            if (text == NULL)
                return;
            out_text_relative(f, color, x, y, dx, dy, text, first, text->length());
        }

        void ISurface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
        {
        }

        void ISurface::line(const Color &c, float x0, float y0, float x1, float y1, float width)
        {
        }

        void ISurface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
        {
        }

        void ISurface::parametric_line(const Color &color, float a, float b, float c, float width)
        {
        }

        void ISurface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
        {
        }

        void ISurface::parametric_bar(
            IGradient *g,
            float a1, float b1, float c1, float a2, float b2, float c2,
            float left, float right, float top, float bottom)
        {
        }

        void ISurface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
        {
        }

        void ISurface::fill_frame(
            const Color &color,
            size_t flags, float radius,
            float fx, float fy, float fw, float fh,
            float ix, float iy, float iw, float ih)
        {
        }

        void ISurface::fill_frame(
                const Color &color, size_t flags, float radius,
                const ws::rectangle_t *out, const ws::rectangle_t *in
            )
        {
            fill_frame(
                color, flags, radius,
                out->nLeft, out->nTop, out->nWidth, out->nHeight,
                in->nLeft, in->nTop, in->nWidth, in->nHeight);
        }

        void ISurface::fill_poly(const Color & color, const float *x, const float *y, size_t n)
        {
        }

        void ISurface::fill_poly(IGradient *gr, const float *x, const float *y, size_t n)
        {
        }

        void ISurface::wire_poly(const Color & color, float width, const float *x, const float *y, size_t n)
        {
        }

        void ISurface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
        {
        }

        void ISurface::fill_circle(const Color & c, float x, float y, float r)
        {
        }

        void ISurface::fill_circle(IGradient *g, float x, float y, float r)
        {
        }

        void ISurface::clip_begin(float x, float y, float w, float h)
        {
        }

        void ISurface::clip_begin(const ws::rectangle_t *area)
        {
            clip_begin(area->nLeft, area->nTop, area->nWidth, area->nHeight);
        }

        void ISurface::clip_end()
        {
        }
    
        bool ISurface::get_antialiasing()
        {
            return false;
        }
    
        bool ISurface::set_antialiasing(bool set)
        {
            return false;
        }

        ws::point_t ISurface::set_origin(const ws::point_t & origin)
        {
            return set_origin(origin.nLeft, origin.nTop);
        }

        ws::point_t ISurface::set_origin(ssize_t left, ssize_t top)
        {
            const ws::point_t result = { 0, 0};
            return result;
        }

    } /* namespace ws */
} /* namespace lsp */
