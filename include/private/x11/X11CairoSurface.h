/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef UI_X11_X11CAIROSURFACE_H_
#define UI_X11_X11CAIROSURFACE_H_

#include <lsp-plug.in/ws/version.h>
#include <lsp-plug.in/common/types.h>

#include <private/x11/X11Display.h>

#ifdef USE_LIBCAIRO

#include <lsp-plug.in/runtime/Color.h>
#include <lsp-plug.in/ws/IGradient.h>
#include <lsp-plug.in/ws/ISurface.h>

#include <cairo/cairo.h>
#include <X11/Xlib.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class X11CairoSurface: public ISurface
            {
                protected:
                    cairo_surface_t        *pSurface;
                    cairo_t                *pCR;
                    cairo_font_options_t   *pFO;
                    cairo_font_face_t      *pFF;
                    X11Display             *pDisplay;
                    bool                    bBegin;

                protected:
                    typedef struct font_context_t
                    {
                        X11Display::font_t     *font;       // Custom font (if present)
                        cairo_font_face_t      *face;       // Selected font face
                        cairo_font_options_t   *opt;        // Font options
                    } font_context_t;

                protected:
                    void                destroy_context();

                    inline void         setSourceRGB(const Color &col);
                    inline void         setSourceRGBA(const Color &col);
                    void                drawRoundRect(float left, float top, float width, float height, float radius, size_t mask);
                    void                set_current_font(font_context_t *ctx, const Font &f);
                    void                unset_current_font(font_context_t *ctx);

                public:
                    /** Create XLib surface
                     *
                     * @param dpy display
                     * @param drawabledrawable
                     * @param visual visual
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11CairoSurface(X11Display *dpy, Drawable drawable, Visual *visual, size_t width, size_t height);

                    /** Create image surface
                     *
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11CairoSurface(X11Display *dpy, size_t width, size_t height);

                    /** Destructor
                     *
                     */
                    virtual ~X11CairoSurface();

                public:
                    /** resize cairo surface if possible
                     *
                     * @param width new width
                     * @param height new height
                     * @return true on success
                     */
                    bool resize(size_t width, size_t height);

                    virtual ISurface *create(size_t width, size_t height);

                    virtual ISurface *create_copy();

                    virtual IGradient *linear_gradient(float x0, float y0, float x1, float y1);

                    virtual IGradient *radial_gradient
                    (
                        float cx0, float cy0, float r0,
                        float cx1, float cy1, float r1
                    );

                    virtual void destroy();

                public:
                    // Drawing methods
                    virtual void draw(ISurface *s, float x, float y);

                    virtual void draw(ISurface *s, float x, float y, float sx, float sy);

                    virtual void draw_alpha(ISurface *s, float x, float y, float sx, float sy, float a);

                    virtual void draw_rotate_alpha(ISurface *s, float x, float y, float sx, float sy, float ra, float a);

                    virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh);

                    virtual void begin();

                    virtual void end();

                    virtual void clear_rgb(uint32_t color);

                    virtual void clear_rgba(uint32_t color);

                    virtual void fill_rect(const Color &color, float left, float top, float width, float height);

                    virtual void fill_rect(IGradient *g, float left, float top, float width, float height);

                    virtual void wire_rect(const Color &color, float left, float top, float width, float height, float line_width);

                    virtual void wire_rect(IGradient *g, float left, float top, float width, float height, float line_width);

                    virtual void wire_round_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                    virtual void wire_round_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                    virtual void wire_round_rect_inside(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                    virtual void wire_round_rect_inside(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width);

                    virtual void fill_round_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height);

                    virtual void fill_round_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r);

                    virtual void fill_round_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height);

                    virtual void fill_round_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r);

                    virtual void full_rect(float left, float top, float width, float height, float line_width, const Color &color);

                    virtual void fill_sector(float cx, float cy, float radius, float angle1, float angle2, const Color &color);

                    virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, IGradient *g);

                    virtual void fill_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const Color &color);

                    virtual void clear(const Color &color);

                    virtual bool get_font_parameters(const Font &f, font_parameters_t *fp);

                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const char *text);

                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last);

                    virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text);

                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last);

                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text);

                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last);

                    virtual void square_dot(float x, float y, float width, const Color &color);

                    virtual void square_dot(float x, float y, float width, float r, float g, float b, float a);

                    virtual void line(float x0, float y0, float x1, float y1, float width, const Color &color);

                    virtual void line(float x0, float y0, float x1, float y1, float width, IGradient *g);

                    virtual void parametric_line(float a, float b, float c, float width, const Color &color);

                    virtual void parametric_line(float a, float b, float c, float left, float right, float top, float bottom, float width, const Color &color);

                    virtual void parametric_bar(float a1, float b1, float c1, float a2, float b2, float c2,
                            float left, float right, float top, float bottom, IGradient *gr);

                    virtual void wire_arc(float x, float y, float r, float a1, float a2, float width, const Color &color);

                    virtual void fill_poly(const Color & color, const float *x, const float *y, size_t n);

                    virtual void fill_poly(IGradient *gr, const float *x, const float *y, size_t n);

                    virtual void wire_poly(const Color & color, float width, const float *x, const float *y, size_t n);

                    virtual void draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n);

                    virtual void fill_circle(float x, float y, float r, const Color & color);

                    virtual void fill_circle(float x, float y, float r, IGradient *g);

                    virtual void clip_begin(float x, float y, float w, float h);

                    void clip_end();

                    virtual void fill_frame(
                        const Color &color,
                        float fx, float fy, float fw, float fh,
                        float ix, float iy, float iw, float ih
                    );

                    virtual void fill_round_frame(
                            const Color &color,
                            float radius, size_t flags,
                            float fx, float fy, float fw, float fh,
                            float ix, float iy, float iw, float ih
                            );

                    virtual bool get_antialiasing();

                    virtual bool set_antialiasing(bool set);

                    virtual surf_line_cap_t get_line_cap();

                    virtual surf_line_cap_t set_line_cap(surf_line_cap_t lc);

                    virtual void *start_direct();

                    virtual void end_direct();
            };
        }
    }

} /* namespace lsp */
#endif /* USE_LIBCAIRO */

#endif /* UI_X11_X11CAIROSURFACE_H_ */
