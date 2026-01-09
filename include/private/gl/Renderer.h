/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 7 янв. 2026 г.
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

#ifndef PRIVATE_GL_RENDERER_H_
#define PRIVATE_GL_RENDERER_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ipc/Condition.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/gl/Allocator.h>
#include <private/gl/Batch.h>
#include <private/gl/SurfaceContext.h>
#include <private/gl/Texture.h>
#include <private/gl/TextAllocator.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class LSP_HIDDEN_MODIFIER Renderer
            {
                protected:
                    enum cmd_color_t
                    {
                        C_SOLID     = 0,
                        C_LINEAR    = 1,
                        C_RADIAL    = 2,
                        C_TEXTURE   = 3
                    };

                protected:
                    typedef struct texture_rect_t
                    {
                        float               sb;
                        float               tb;
                        float               se;
                        float               te;
                    } texture_rect_t;

                    typedef struct texcoord_t
                    {
                        float               x;
                        float               y;
                        float               sx;
                        float               sy;
                    } texcoord_t;

                protected:
                    uatomic_t                       nReferences;
                    gl::IContext                   *pGLContext;
                    ipc::Condition                  sLock;
                    ipc::Thread                     sThread;
                    lltl::parray<SurfaceContext>    sQueue;
                    gl::Allocator                   sAllocator;
                    gl::TextAllocator               sTextAllocator;
                    gl::Batch                       sBatch;
                    gl::rectangle_t                 sViewport;
                    lltl::darray<gl::uniform_t>     vUniforms;

                protected:
                    static status_t execute(void *arg);

                protected:
                    status_t                setup_context(SurfaceContext *surface);
                    status_t                run();
                    void                    do_destroy();
                    SurfaceContext         *poll();
                    status_t                render_batch(SurfaceContext *surface);

                protected: // Drawing
                    static inline float    *copy_coords(const float *x, const float *y, size_t n);
                    static inline ssize_t   make_command(ssize_t index, cmd_color_t color, const gl::clip_state_t & clipping);
                    static inline float    *serialize_clipping(float *dst, const gl::clip_state_t & clipping);
                    static inline float    *serialize_color(float *dst, const gl::color_t & c);
                    static inline void      extend_rect(clip_rect_t & rect, float x, float y);
                    static inline void      limit_rect(clip_rect_t & rect, SurfaceContext * surface);

                    bool                    update_uniforms(SurfaceContext * surface);

                    gl::Texture            *make_text(texture_rect_t *rect, const void *data, size_t width, size_t height, size_t stride);

                    ssize_t                 start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::color_t & color);
                    ssize_t                 start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::linear_gradient_t & g);
                    ssize_t                 start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::radial_gradient_t & g);
                    ssize_t                 start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, gl::Texture *texture, const gl::color_t & color);
                    ssize_t                 start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::fill_t & fill);

                    void                    fill_triangle(uint32_t ci, float x0, float y0, float x1, float y1, float x2, float y2);
                    void                    fill_triangle_fan(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, size_t n);
                    void                    fill_rect(uint32_t ci, float x0, float y0, float x1, float y1);
                    void                    fill_rect_textured(uint32_t ci, const texcoord_t & tex, float x0, float y0, float x1, float y1);
                    void                    fill_circle(uint32_t ci, float x, float y, float r);
                    void                    fill_sector(uint32_t ci, float x, float y, float r, float a1, float a2);
                    void                    fill_sector_textured(uint32_t ci, const texcoord_t & tex, float x, float y, float r, float a1, float a2);
                    void                    fill_corner(uint32_t ci, float x0, float y0, float xd, float yd, float r, float a);

                    void                    wire_line(uint32_t ci, float x0, float y0, float x1, float y1, float width);
                    template <class T>
                    inline void             wire_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, const float *x, const float *y, float width, size_t n);
                    template <class T>
                    inline void             wire_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n);
                    void                    wire_polyline(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n);
                    void                    wire_polyline(uint32_t ci, const float *x, const float *y, float width, size_t n);
                    void                    wire_arc(uint32_t ci, float x, float y, float r, float a1, float a2, float width);

                protected: // Event processing
                    status_t                process(SurfaceContext * surface, const actions::action_t & action);

                    status_t                process(SurfaceContext * surface, const actions::init_t & action);
                    status_t                process(SurfaceContext * surface, const actions::clear_t & action);
                    status_t                process(SurfaceContext * surface, const actions::resize_t & action);
                    status_t                process(SurfaceContext * surface, const actions::draw_surface_t & action);
                    status_t                process(SurfaceContext * surface, const actions::draw_raw_t & action);
                    status_t                process(SurfaceContext * surface, const actions::wire_rect_t & action);
                    status_t                process(SurfaceContext * surface, const actions::fill_rect_t & action);
                    status_t                process(SurfaceContext * surface, const actions::fill_sector_t & action);
                    status_t                process(SurfaceContext * surface, const actions::fill_triangle_t & action);
                    status_t                process(SurfaceContext * surface, const actions::fill_circle_t & action);
                    status_t                process(SurfaceContext * surface, const actions::wire_arc_t & action);
                    status_t                process(SurfaceContext * surface, const actions::out_text_t & action);
                    status_t                process(SurfaceContext * surface, const actions::out_text_bitmap_t & action);
                    status_t                process(SurfaceContext * surface, const actions::out_text_relative_t & action);
                    status_t                process(SurfaceContext * surface, const actions::line_t & action);
                    status_t                process(SurfaceContext * surface, const actions::parametric_line_t & action);
                    status_t                process(SurfaceContext * surface, const actions::parametric_bar_t & action);
                    status_t                process(SurfaceContext * surface, const actions::fill_frame_t & action);
                    status_t                process(SurfaceContext * surface, const actions::draw_poly_t & action);
                    status_t                process(SurfaceContext * surface, const actions::clip_begin_t & action);
                    status_t                process(SurfaceContext * surface, const actions::clip_end_t & action);
                    status_t                process(SurfaceContext * surface, const actions::set_antialiasing_t & action);
                    status_t                process(SurfaceContext * surface, const actions::set_origin_t & action);

                public:
                    explicit Renderer(gl::IContext * gl_context);
                    Renderer(const Renderer &) = delete;
                    Renderer(Renderer &&) = delete;
                    virtual ~Renderer();

                    Renderer & operator = (const Renderer &) = delete;
                    Renderer & operator = (Renderer &&) = delete;

                    virtual status_t        init();
                    virtual void            destroy();

                public:
                    uatomic_t               reference_up();
                    uatomic_t               reference_down();

                public:
                    status_t                queue_draw(SurfaceContext * surface);

            };
        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_RENDERER_H_ */
