/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 15 янв. 2025 г.
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

#ifndef PRIVATE_X11_X11GLSURFACE_H_
#define PRIVATE_X11_X11GLSURFACE_H_

#include <lsp-plug.in/ws/version.h>

#if defined(USE_LIBX11)

#include <lsp-plug.in/common/types.h>

#include <private/x11/X11Display.h>

#include <lsp-plug.in/runtime/Color.h>
#include <lsp-plug.in/ws/IGradient.h>
#include <lsp-plug.in/ws/ISurface.h>

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

namespace lsp
{
    namespace ws
    {
        namespace x11
        {
            class LSP_HIDDEN_MODIFIER X11GLSurface: public ISurface
            {
                protected:
                    X11Display             *pDisplay;
                    GLXContext              pContext;
                #ifdef LSP_DEBUG
                    size_t                  nNumClips;
                #endif /* LSP_DEBUG */

                public:
                    /** Create GL surface
                     *
                     * @param dpy display
                     * @param drawabledrawable
                     * @param visual visual
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11GLSurface(X11Display *dpy, Drawable drawable, Visual *visual, size_t width, size_t height);

                    /** Create image surface
                     *
                     * @param context nested glx context
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit X11GLSurface(X11Display *dpy, GLXContext context, size_t width, size_t height);

                    X11GLSurface(const X11GLSurface &) = delete;
                    X11GLSurface(X11GLSurface &&) = delete;
                    virtual ~X11GLSurface();

                    X11GLSurface & operator = (const X11GLSurface &) = delete;
                    X11GLSurface & operator = (X11GLSurface &&) = delete;

                    virtual void destroy() override;
                    virtual bool valid() const override;

                public:
                    virtual IDisplay *display() override;

                    virtual ISurface *create(size_t width, size_t height) override;

                    virtual IGradient *linear_gradient(float x0, float y0, float x1, float y1) override;
                    virtual IGradient *radial_gradient
                    (
                        float cx0, float cy0,
                        float cx1, float cy1,
                        float r
                    ) override;

                public:
                    // Drawing methods
                    virtual void draw(ISurface *s, float x, float y, float sx, float sy, float a) override;
                    virtual void draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a) override;
                    virtual void draw_clipped(ISurface *s, float x, float y, float sx, float sy, float sw, float sh, float a) override;
                    virtual void draw_raw(
                        const void *data, size_t width, size_t height, size_t stride,
                        float x, float y, float sx, float sy, float a) override;

                    virtual void begin() override;
                    virtual void end() override;

                    virtual void clear(const Color &color) override;
                    virtual void clear_rgb(uint32_t color) override;
                    virtual void clear_rgba(uint32_t color) override;

                    virtual void wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width) override;
                    virtual void wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width) override;

                    virtual void fill_rect(const Color &color, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_rect(const Color &color, size_t mask, float radius, const ws::rectangle_t *r) override;
                    virtual void fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height) override;
                    virtual void fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r) override;

                    virtual void fill_sector(const Color &c, float cx, float cy, float radius, float angle1, float angle2) override;
                    virtual void fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2) override;
                    virtual void fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2) override;
                    virtual void fill_circle(const Color &c, float x, float y, float r) override;
                    virtual void fill_circle(IGradient *g, float x, float y, float r) override;
                    virtual void wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width) override;

                    virtual bool get_font_parameters(const Font &f, font_parameters_t *fp) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const char *text) override;
                    virtual bool get_text_parameters(const Font &f, text_parameters_t *tp, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const char *text) override;
                    virtual void out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text) override;
                    virtual void out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last) override;

                    virtual void line(const Color &c, float x0, float y0, float x1, float y1, float width) override;
                    virtual void line(IGradient *g, float x0, float y0, float x1, float y1, float width) override;

                    virtual void parametric_line(const Color &color, float a, float b, float c, float width) override;
                    virtual void parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width) override;

                    virtual void parametric_bar(
                        IGradient *g,
                        float a1, float b1, float c1, float a2, float b2, float c2,
                        float left, float right, float top, float bottom) override;

                    virtual void fill_poly(const Color & color, const float *x, const float *y, size_t n) override;
                    virtual void fill_poly(IGradient *gr, const float *x, const float *y, size_t n) override;
                    virtual void wire_poly(const Color & color, float width, const float *x, const float *y, size_t n) override;
                    virtual void draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n) override;

                    virtual void clip_begin(float x, float y, float w, float h) override;
                    virtual void clip_end() override;

                    virtual void fill_frame(
                        const Color &color,
                        size_t flags, float radius,
                        float fx, float fy, float fw, float fh,
                        float ix, float iy, float iw, float ih) override;

                    virtual bool get_antialiasing() override;
                    virtual bool set_antialiasing(bool set) override;

                    virtual surf_line_cap_t get_line_cap() override;
                    virtual surf_line_cap_t set_line_cap(surf_line_cap_t lc) override;
            };

        } /* namespace x11 */
    } /* namespace ws */
} /* namespace lsp */

#endif /* defined(USE_LIBX11) */



#endif /* PRIVATE_X11_X11GLSURFACE_H_ */
