/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 2 янв. 2026 г.
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

#ifndef PRIVATE_GL_ACTIONS_H_
#define PRIVATE_GL_ACTIONS_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/ws/Font.h>
#include <lsp-plug.in/ws/IGradient.h>
#include <lsp-plug.in/runtime/Color.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class SurfaceContext;

            // Color
            typedef struct color_t
            {
                float r, g, b, a;
            } color_t;

            // Gradient
            typedef struct base_gradient_t
            {
                color_t         start;
                color_t         end;
            } base_gradient_t;

            // Linear gradient parameters
            typedef struct linear_gradient_t
            {
                color_t         start;
                color_t         end;
                float           x1, y1;
                float           x2, y2;
            } linear_gradient_t;

            // Radial gradient parameters
            typedef struct radial_gradient_t
            {
                color_t         start;
                color_t         end;
                float           x1, y1;
                float           x2, y2;
                float           r;
            } radial_gradient_t;

            typedef struct texture_t
            {
                SurfaceContext *surface;
                float           alpha;
            } texture_t;

            enum fill_type_t
            {
                FILL_NONE,
                FILL_SOLID_COLOR,
                FILL_LINEAR_GRADIENT,
                FILL_RADIAL_GRADIENT,
                FILL_TEXTURE
            };

            typedef struct fill_t
            {
                fill_type_t      type;
                union
                {
                    color_t             color;
                    base_gradient_t     gradient;
                    linear_gradient_t   linear;
                    radial_gradient_t   radial;
                    texture_t           texture;
                };
            } fill_t;

            typedef struct clip_rect_t
            {
                float               left;
                float               top;
                float               right;
                float               bottom;
            } clip_rect_t;

            typedef struct origin_t
            {
                int32_t             left;
                int32_t             top;
            } origin_t;

            typedef struct surface_size_t
            {
                uint32_t            width;
                uint32_t            height;
            } surface_size_t;

            typedef struct matrix_t
            {
                float               v[16];
            } matrix_t;

            typedef struct clip_state_t
            {
                static constexpr size_t MAX_CLIPS       = 8;

                clip_rect_t         clips[MAX_CLIPS];
                uint32_t            count;
            } clip_state_t;

            typedef struct rectangle_t
            {
                float               x;
                float               y;
                float               width;
                float               height;
            } rectangle_t;

            // Actions

            namespace actions
            {
                enum action_type_t
                {
                    INIT,
                    CLEAR,
                    RESIZE,
                    DRAW_SURFACE,
                    DRAW_RAW,
                    WIRE_RECT,
                    FILL_RECT,
                    FILL_SECTOR,
                    FILL_TRIANGLE,
                    FILL_CIRCLE,
                    WIRE_ARC,
                    OUT_TEXT,
                    OUT_TEXT_RELATIVE,
                    LINE,
                    PARAMETRIC_LINE,
                    PARAMETRIC_BAR,
                    FILL_FRAME,
                    DRAW_POLY,
                    CLIP_BEGIN,
                    CLIP_END,
                    SET_ANTIALIASING,
                    SET_ORIGIN
                };

                typedef struct init_t
                {
                    static constexpr action_type_t type_id = action_type_t::INIT;

                    surface_size_t  size;
                    origin_t        origin;
                    bool            antialiasing;
                } init_t;

                typedef struct clear_t
                {
                    static constexpr action_type_t type_id = action_type_t::CLEAR;

                    gl::color_t     color;
                } clear_t;

                typedef struct resize_t
                {
                    static constexpr action_type_t type_id = action_type_t::RESIZE;

                    surface_size_t  size;
                } resize_t;

                typedef struct draw_surface_t
                {
                    static constexpr action_type_t type_id = action_type_t::DRAW_SURFACE;

                    SurfaceContext *surface;
                    float           x;
                    float           y;
                    float           scale_x;
                    float           scale_y;
                    float           angle;
                    float           alpha;
                } draw_surface_t;

                typedef struct draw_raw_t
                {
                    static constexpr action_type_t type_id = action_type_t::DRAW_RAW;

                    void           *data;
                    uint32_t        width;
                    uint32_t        height;
                    uint32_t        stride;
                    float           x;
                    float           y;
                    const float     src_x;
                    const float     src_y;
                    const float     alpha;
                } draw_raw_t;

                typedef struct wire_rect_t
                {
                    static constexpr action_type_t type_id = action_type_t::WIRE_RECT;

                    gl::fill_t      fill;
                    gl::rectangle_t rectangle;
                    float           radius;
                    float           line_width;
                    uint32_t        corners;
                } wire_rect_color_t;


                typedef struct fill_rect_t
                {
                    static constexpr action_type_t type_id = action_type_t::FILL_RECT;

                    gl::fill_t      fill;
                    gl::rectangle_t rectangle;
                    float           radius;
                    uint32_t        corners;
                } fill_rect_t;

                typedef struct fill_sector_t
                {
                    static constexpr action_type_t type_id = action_type_t::FILL_SECTOR;

                    gl::fill_t      fill;
                    float           center_x;
                    float           center_y;
                    float           radius;
                    float           angle_start;
                    float           angle_end;
                } fill_sector_t;

                typedef struct fill_triangle_t
                {
                    static constexpr action_type_t type_id = action_type_t::FILL_TRIANGLE;

                    gl::fill_t      fill;
                    float           x[3];
                    float           y[3];
                } fill_triangle_t;

                typedef struct fill_circle_t
                {
                    static constexpr action_type_t type_id = action_type_t::FILL_CIRCLE;

                    gl::fill_t      fill;
                    float           center_x;
                    float           center_y;
                    float           radius;
                } fill_circle_t;

                typedef struct wire_arc_t
                {
                    static constexpr action_type_t type_id = action_type_t::WIRE_ARC;

                    gl::fill_t      fill;
                    float           center_x;
                    float           center_y;
                    float           radius;
                    float           angle_start;
                    float           angle_end;
                    float           width;
                } wire_arc_t;

                typedef struct out_text_t
                {
                    static constexpr action_type_t type_id = action_type_t::OUT_TEXT;

                    gl::color_t     fill;
                    ws::Font        font;
                    LSPString       text;
                    float           x;
                    float           y;
                } out_text_t;

                typedef struct out_text_relative_t
                {
                    static constexpr action_type_t type_id = action_type_t::OUT_TEXT_RELATIVE;

                    gl::color_t     fill;
                    ws::Font        font;
                    LSPString       text;
                    float           x;
                    float           y;
                    float           relative_x;
                    float           relative_y;
                } out_text_relative_t;

                typedef struct line_t
                {
                    static constexpr action_type_t type_id = action_type_t::LINE;

                    gl::fill_t      fill;
                    float           x[2];
                    float           y[2];
                    float           width;
                } line_t;

                typedef struct parametric_line_t
                {
                    static constexpr action_type_t type_id = action_type_t::PARAMETRIC_LINE;

                    gl::fill_t      fill;
                    gl::clip_rect_t rect;
                    float           a;
                    float           b;
                    float           c;
                    float           width;
                } parametric_line_t;

                typedef struct parametric_bar_t
                {
                    static constexpr action_type_t type_id = action_type_t::PARAMETRIC_BAR;

                    gl::fill_t      fill;
                    gl::clip_rect_t rect;
                    float           a[2];
                    float           b[2];
                    float           c[2];

                } parametric_bar_t;

                typedef struct fill_frame_t
                {
                    static constexpr action_type_t type_id = action_type_t::FILL_FRAME;

                    gl::fill_t      fill;
                    gl::rectangle_t outer_rect;
                    gl::rectangle_t inner_rect;
                    float           radius;
                    uint32_t        corners;
                } fill_frame_t;

                typedef struct draw_poly_t
                {
                    static constexpr action_type_t type_id = action_type_t::DRAW_POLY;

                    gl::fill_t      fill;
                    gl::fill_t      wire;
                    float          *data;
                    float           width;
                    uint32_t        count;
                } draw_poly_t;

                typedef struct clip_begin_t
                {
                    static constexpr action_type_t type_id = action_type_t::CLIP_BEGIN;

                    gl::clip_rect_t rect;
                } clip_begin_t;

                typedef struct clip_end_t
                {
                    static constexpr action_type_t type_id = action_type_t::CLIP_END;
                } clip_end_t;

                typedef struct set_antialiasing_t
                {
                    static constexpr action_type_t type_id = action_type_t::SET_ANTIALIASING;
                    bool            enable;
                } set_antialiasing_t;

                typedef struct set_origin_t
                {
                    static constexpr action_type_t type_id = action_type_t::SET_ORIGIN;
                    gl::origin_t    origin;
                } set_origin_t;

                typedef struct action_t
                {
                    action_type_t   type;
                    union
                    {
                        init_t                      init;
                        clear_t                     clear;
                        resize_t                    resize;
                        draw_surface_t              draw_surface;
                        draw_raw_t                  draw_raw;
                        wire_rect_t                 wire_rect;
                        fill_rect_t                 fill_rect;
                        fill_sector_t               fill_sector;
                        fill_triangle_t             fill_triangle;
                        fill_circle_t               fill_circle;
                        wire_arc_t                  wire_arc;
                        out_text_t                  out_text;
                        out_text_relative_t         out_text_relative;
                        line_t                      line;
                        parametric_line_t           parametric_line;
                        parametric_bar_t            parametric_bar;
                        fill_frame_t                fill_frame;
                        draw_poly_t                 draw_poly;
                        clip_begin_t                clip_begin;
                        clip_end_t                  clip_end;
                        set_antialiasing_t          set_antialiasing;
                        set_origin_t                set_origin;
                        uint8_t                     data[];
                    };
                } action_t;

                action_t *init(action_t *action, action_type_t type);
                void destroy(action_t *action);

            } /* namespace actions */

            void set_color(color_t & color, const lsp::Color & c);
            void set_fill(fill_t & fill, const lsp::Color & c);
            void set_fill(fill_t & fill, const IGradient * g);
            void set_fill(fill_t & fill, SurfaceContext * ctx, float alpha);

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */



#endif /* PRIVATE_GL_ACTIONS_H_ */
