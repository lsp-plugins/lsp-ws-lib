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

#include <lsp-plug.in/common/types.h>

#include <private/gl/IContext.h>
#include <private/gl/Batch.h>

#include <lsp-plug.in/runtime/Color.h>
#include <lsp-plug.in/ws/IGradient.h>
#include <lsp-plug.in/ws/ISurface.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            /**
             * Drawing surface
             */
            class LSP_HIDDEN_MODIFIER Surface: public ISurface
            {
                private:
                    enum cmd_color_t
                    {
                        C_FIXED     = 0,
                        C_LINEAR    = 1,
                        C_RADIAL    = 2,
                        C_TEXTURE   = 3
                    };

                    typedef struct clip_rect_t
                    {
                        float               left;
                        float               top;
                        float               right;
                        float               bottom;
                    } clip_rect_t;

                    typedef struct color_t
                    {
                        float               r, g, b, a;
                    } color_t;

                    static constexpr size_t MAX_CLIPS       = 8;

                protected:
                    IDisplay               *pDisplay;
                    gl::IContext           *pContext;
                    gl::Batch               sBatch;
                    size_t                  nWidth;
                    size_t                  nHeight;
                    size_t                  nNumClips;
                    float                   vMatrix[16];
                    clip_rect_t             vClips[MAX_CLIPS];

                    bool                    bNested;
                    bool                    bIsDrawing;         // Surface is currently in drawing mode
                    bool                    bAntiAliasing;      // Anti-aliasing option

                private:
                    void do_destroy();

                private:
                    /** Create nexted GL surface
                     *
                     * @param ctx OpenGL context
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit Surface(size_t width, size_t height);

                private:
                    ssize_t start_batch(batch_program_t program, const Color & color);
                    ssize_t start_batch(batch_program_t program, float r, float g, float b, float a);
                    inline ssize_t make_command(ssize_t index, cmd_color_t color) const;

                    inline float *serialize_clipping(float *dst) const;
                    static inline float *serialize_color(float *dst, float r, float g, float b, float a);
                    static inline float *serialize_color(float *dst, const Color & c);

                public:
                    /** Create GL surface
                     *
                     * @param display associated display
                     * @param ctx OpenGL context
                     * @param width surface width
                     * @param height surface height
                     */
                    explicit Surface(IDisplay *display, gl::IContext *ctx, size_t width, size_t height);

                    Surface(const Surface &) = delete;
                    Surface(Surface &&) = delete;
                    virtual ~Surface();

                    Surface & operator = (const Surface &) = delete;
                    Surface & operator = (Surface &&) = delete;

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
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_X11_X11GLSURFACE_H_ */
