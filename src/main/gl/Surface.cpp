/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <private/freetype/FontManager.h>
#include <private/gl/Batch.h>
#include <private/gl/Gradient.h>
#include <private/gl/Stats.h>
#include <private/gl/Surface.h>
#include <private/x11/X11Display.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            constexpr float k_color = 1.0f / 255.0f;
            constexpr float k_alpha_color = 254.0f / 255.0f;
            
            Surface::Surface(IDisplay *display, SurfaceContext * context):
                ISurface(context->width(), context->height(), ST_OPENGL),
                pDisplay(display),
                pSurface(safe_acquire(context))
            {
                OPENGL_INC_STATS(surface_alloc);

                bAntiAliasing   = pSurface->antialiasing();
                sClipping       = pSurface->clipping();
                sOrigin         = pSurface->origin();

                lsp_trace("OpenGL surface created ptr=%p", this);
            }

            IDisplay *Surface::display()
            {
                return pDisplay;
            }

            ISurface *Surface::create(size_t width, size_t height)
            {
                if ((pSurface == NULL) || (!pSurface->valid()))
                    return NULL;

                gl::SurfaceContext *surface = new gl::SurfaceContext(pSurface, width, height);
                if (surface == NULL)
                    return NULL;
                lsp_finally { safe_release(surface); };

                Surface *s = new Surface(pDisplay, surface);
                if (s == NULL)
                    return NULL;

                return s;
            }

            IGradient *Surface::linear_gradient(float x0, float y0, float x1, float y1)
            {
                return new gl::Gradient(
                    gl::linear_gradient_t {
                        { 0.0f, 0.0f, 0.0f, 1.0f, },
                        { 1.0f, 1.0f, 1.0f, 0.0f, },
                        x0, y0,
                        x1, y1
                    });
            }

            IGradient *Surface::radial_gradient(float cx0, float cy0, float cx1, float cy1, float r)
            {
                return new gl::Gradient(
                    gl::radial_gradient_t {
                        { 0.0f, 0.0f, 0.0f, 1.0f, },
                        { 1.0f, 1.0f, 1.0f, 0.0f, },
                        cx0, cy0,
                        cx1, cy1,
                        r
                    });
            }

            void Surface::do_destroy()
            {
                if (pSurface != NULL)
                {
                    pSurface->invalidate();
                    safe_release(pSurface);
                }
            }

            Surface::~Surface()
            {
                OPENGL_INC_STATS(surface_free);

                do_destroy();

                OPENGL_OUTPUT_STATS(true);
            }

            void Surface::destroy()
            {
                do_destroy();
            }

            bool Surface::valid() const
            {
                return (pSurface != NULL) && (pSurface->valid());
            }

            void Surface::draw(ISurface *s, float x, float y, float sx, float sy, float a)
            {
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                if (gls->pSurface == NULL)
                    return;
                if (gls->pSurface->is_drawing())
                    return;

                gl::actions::draw_surface_t *cmd = pSurface->append_command<gl::actions::draw_surface_t>();

                set_fill(cmd->fill, gls->pSurface, a);

                cmd->x              = x;
                cmd->y              = y;
                cmd->scale_x        = sx;
                cmd->scale_y        = sy;
                cmd->angle          = 0.0f;
            }

            void Surface::draw_rotate(ISurface *s, float x, float y, float sx, float sy, float ra, float a)
            {
                if (s->type() != ST_OPENGL)
                    return;

                gl::Surface *gls = static_cast<gl::Surface *>(s);
                if (gls->pSurface == NULL)
                    return;
                if (gls->pSurface->is_drawing())
                    return;

                gl::actions::draw_surface_t *cmd = pSurface->append_command<gl::actions::draw_surface_t>();

                set_fill(cmd->fill, gls->pSurface, a);
                cmd->x              = x;
                cmd->y              = y;
                cmd->scale_x        = sx;
                cmd->scale_y        = sy;
                cmd->angle          = ra;
            }

            void Surface::draw_raw(
                const void *data, size_t width, size_t height, size_t stride,
                float x, float y, float sx, float sy, float a)
            {
                if (!pSurface->is_drawing())
                    return;

                if ((fabsf(width*sx) <= 1e-3) || (fabsf(height*sy) <= 1e-3) || (a >= k_alpha_color))
                    return;

                // Copy contents for drawing
                const size_t x_bytes            = lsp_max(width * sizeof(uint32_t), stride);
                const size_t total_bytes        = x_bytes * height;
                void *copy                      = malloc(total_bytes);
                if (copy == NULL)
                    return;
                lsp_finally {
                    if (copy != NULL)
                        free(copy);
                };
                memcpy(copy, data, total_bytes);

                // Emit command
                gl::actions::draw_raw_t *cmd    = pSurface->append_command<gl::actions::draw_raw_t>();
                if (cmd == NULL)
                    return;

                cmd->data       = release_ptr(copy);
                cmd->blend.r    = 1.0f;
                cmd->blend.g    = 1.0f;
                cmd->blend.b    = 1.0f;
                cmd->blend.a    = a;
                cmd->width      = uint32_t(width);
                cmd->height     = uint32_t(height);
                cmd->stride     = uint32_t(stride);
                cmd->x          = x;
                cmd->y          = y;
                cmd->scale_x    = sx;
                cmd->scale_y    = sy;
            }

            status_t Surface::resize(size_t width, size_t height)
            {
                if ((nWidth == width) && (nHeight == height))
                    return STATUS_OK;

                nWidth      = width;
                nHeight     = height;

                gl::actions::resize_t * const cmd = pSurface->append_command<gl::actions::resize_t>();
                if (cmd != NULL)
                {
                    cmd->size.width     = uint32_t(nWidth);
                    cmd->size.height    = uint32_t(nHeight);
                }

                return STATUS_OK;
            }

            void Surface::begin()
            {
                if (!pSurface->begin_draw())
                    return;

                // Emit initialization command
                gl::actions::init_t * const cmd = pSurface->append_command<gl::actions::init_t>();
                if (cmd != NULL)
                {
                    cmd->size.width     = nWidth;
                    cmd->size.height    = nHeight;
                    cmd->origin         = sOrigin;
                    cmd->antialiasing   = bAntiAliasing;
                }

            #ifdef LSP_DEBUG
                sClipping.count = 0;
            #endif /* LSP_DEBUG */
            }

            bool Surface::ready() const
            {
                return !pSurface->is_rendering();
            }

            void Surface::wait()
            {
                return pSurface->wait();
            }

            void Surface::end()
            {
                pSurface->end_draw();
            }

            void Surface::clear_rgb(uint32_t rgb)
            {
                gl::actions::clear_t * const cmd = pSurface->append_command<gl::actions::clear_t>();
                if (cmd == NULL)
                    return;

                cmd->color.r = float((rgb >> 16) & 0xff) * k_color;
                cmd->color.g = float((rgb >> 8) & 0xff) * k_color;
                cmd->color.b = float(rgb & 0xff) * k_color;
                cmd->color.a = 0.0f;
            }

            void Surface::clear_rgba(uint32_t rgba)
            {
                gl::actions::clear_t * const cmd = pSurface->append_command<gl::actions::clear_t>();
                if (cmd == NULL)
                    return;

                cmd->color.r = float((rgba >> 16) & 0xff) * k_color;
                cmd->color.g = float((rgba >> 8) & 0xff) * k_color;
                cmd->color.b = float(rgba & 0xff) * k_color;
                cmd->color.a = float((rgba >> 24) & 0xff) * k_color;
            }

            void Surface::clear(const Color & c)
            {
                gl::actions::clear_t * const cmd = pSurface->append_command<gl::actions::clear_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->color, c);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                gl::actions::wire_rect_t * const cmd = pSurface->append_command<gl::actions::wire_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->rectangle.x        = left;
                cmd->rectangle.y        = top;
                cmd->rectangle.width    = width;
                cmd->rectangle.height   = height;
                cmd->radius             = radius;
                cmd->line_width         = line_width;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::wire_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                gl::actions::wire_rect_t * const cmd = pSurface->append_command<gl::actions::wire_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->rectangle.x        = r->nLeft;
                cmd->rectangle.y        = r->nTop;
                cmd->rectangle.width    = r->nWidth;
                cmd->rectangle.height   = r->nHeight;
                cmd->radius             = radius;
                cmd->line_width         = line_width;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r, float line_width)
            {
                gl::actions::wire_rect_t * const cmd = pSurface->append_command<gl::actions::wire_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->rectangle.x        = r->nLeft;
                cmd->rectangle.y        = r->nTop;
                cmd->rectangle.width    = r->nWidth;
                cmd->rectangle.height   = r->nHeight;
                cmd->radius             = radius;
                cmd->line_width         = line_width;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::wire_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height, float line_width)
            {
                gl::actions::wire_rect_t * const cmd = pSurface->append_command<gl::actions::wire_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->rectangle.x        = left;
                cmd->rectangle.y        = top;
                cmd->rectangle.width    = width;
                cmd->rectangle.height   = height;
                cmd->radius             = radius;
                cmd->line_width         = line_width;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, float left, float top, float width, float height)
            {
                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->rectangle.x        = left;
                cmd->rectangle.y        = top;
                cmd->rectangle.width    = width;
                cmd->rectangle.height   = height;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(const Color &c, size_t mask, float radius, const ws::rectangle_t *r)
            {
                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->rectangle.x        = r->nLeft;
                cmd->rectangle.y        = r->nTop;
                cmd->rectangle.width    = r->nWidth;
                cmd->rectangle.height   = r->nHeight;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, float left, float top, float width, float height)
            {
                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->rectangle.x        = left;
                cmd->rectangle.y        = top;
                cmd->rectangle.width    = width;
                cmd->rectangle.height   = height;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(IGradient *g, size_t mask, float radius, const ws::rectangle_t *r)
            {
                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->rectangle.x        = r->nLeft;
                cmd->rectangle.y        = r->nTop;
                cmd->rectangle.width    = r->nWidth;
                cmd->rectangle.height   = r->nHeight;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, float left, float top, float width, float height)
            {
                if (s->type() != ST_OPENGL)
                    return;
                gl::Surface *gls = static_cast<gl::Surface *>(s);
                if (gls->pSurface->is_drawing())
                    return;

                // Create texture
                if (!pSurface->is_drawing())
                    return;

                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, gls->pSurface, alpha);
                cmd->rectangle.x        = left;
                cmd->rectangle.y        = top;
                cmd->rectangle.width    = width;
                cmd->rectangle.height   = height;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_rect(ISurface *s, float alpha, size_t mask, float radius, const ws::rectangle_t *r)
            {
                if (s->type() != ST_OPENGL)
                    return;
                gl::Surface *gls = static_cast<gl::Surface *>(s);
                if (gls->pSurface->is_drawing())
                    return;

                // Create texture
                if (!pSurface->is_drawing())
                    return;

                gl::actions::fill_rect_t * const cmd = pSurface->append_command<gl::actions::fill_rect_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, gls->pSurface, alpha);
                cmd->rectangle.x        = r->nLeft;
                cmd->rectangle.y        = r->nTop;
                cmd->rectangle.width    = r->nWidth;
                cmd->rectangle.height   = r->nHeight;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(mask);
            }

            void Surface::fill_sector(const Color & c, float x, float y, float r, float a1, float a2)
            {
                if (r <= 0.0f)
                    return;

                gl::actions::fill_sector_t * const cmd = pSurface->append_command<gl::actions::fill_sector_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->center_x   = x;
                cmd->center_y   = y;
                cmd->radius     = r;
                cmd->angle_start= a1;
                cmd->angle_end  = a2;
            }

            void Surface::fill_triangle(IGradient *g, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                gl::actions::fill_triangle_t * const cmd = pSurface->append_command<gl::actions::fill_triangle_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->x[0]   = x0;
                cmd->x[1]   = x1;
                cmd->x[2]   = x2;
                cmd->y[0]   = y0;
                cmd->y[1]   = y1;
                cmd->y[2]   = y2;
            }

            void Surface::fill_triangle(const Color &c, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                gl::actions::fill_triangle_t * const cmd = pSurface->append_command<gl::actions::fill_triangle_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->x[0]   = x0;
                cmd->x[1]   = x1;
                cmd->x[2]   = x2;
                cmd->y[0]   = y0;
                cmd->y[1]   = y1;
                cmd->y[2]   = y2;
            }

            void Surface::line(const Color & c, float x0, float y0, float x1, float y1, float width)
            {
                gl::actions::line_t * const cmd = pSurface->append_command<gl::actions::line_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->x[0]   = x0;
                cmd->x[1]   = x1;
                cmd->y[0]   = y0;
                cmd->y[1]   = y1;
                cmd->width  = width;
            }

            void Surface::line(IGradient *g, float x0, float y0, float x1, float y1, float width)
            {
                gl::actions::line_t * const cmd = pSurface->append_command<gl::actions::line_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->x[0]   = x0;
                cmd->x[1]   = x1;
                cmd->y[0]   = y0;
                cmd->y[1]   = y1;
                cmd->width  = width;
            }

            void Surface::parametric_line(const Color & color, float a, float b, float c, float width)
            {
                gl::actions::parametric_line_t * const cmd = pSurface->append_command<gl::actions::parametric_line_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, color);
                cmd->rect.left  = 0.0f;
                cmd->rect.top   = 0.0f;
                cmd->rect.right = nWidth;
                cmd->rect.bottom= nHeight;
                cmd->a = a;
                cmd->b = b;
                cmd->c = c;
                cmd->width = width;
            }

            void Surface::parametric_line(const Color &color, float a, float b, float c, float left, float right, float top, float bottom, float width)
            {
                gl::actions::parametric_line_t * const cmd = pSurface->append_command<gl::actions::parametric_line_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, color);
                cmd->rect.left  = left;
                cmd->rect.top   = top;
                cmd->rect.right = right;
                cmd->rect.bottom= bottom;
                cmd->a = a;
                cmd->b = b;
                cmd->c = c;
                cmd->width = width;
            }

            void Surface::parametric_bar(
                IGradient *g,
                float a1, float b1, float c1, float a2, float b2, float c2,
                float left, float right, float top, float bottom)
            {
                gl::actions::parametric_bar_t * const cmd = pSurface->append_command<gl::actions::parametric_bar_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->rect.left  = left;
                cmd->rect.top   = top;
                cmd->rect.right = right;
                cmd->rect.bottom= bottom;
                cmd->a[0] = a1;
                cmd->a[1] = a2;
                cmd->b[0] = b1;
                cmd->b[1] = b2;
                cmd->c[0] = c1;
                cmd->c[1] = c2;
            }

            void Surface::wire_arc(const Color &c, float x, float y, float r, float a1, float a2, float width)
            {
                if (r <= 0.0f)
                    return;

                gl::actions::wire_arc_t * const cmd = pSurface->append_command<gl::actions::wire_arc_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->center_x   = x;
                cmd->center_y   = y;
                cmd->radius     = r;
                cmd->angle_start= a1;
                cmd->angle_end  = a2;
                cmd->width      = width;
            }

            inline float *Surface::copy_coords(const float *x, const float *y, size_t n)
            {
                float *res = static_cast<float *>(malloc(n * 2 * sizeof(float)));
                if (res == NULL)
                    return NULL;

                memcpy(res, x, n*sizeof(float));
                memcpy(&res[n], y, n*sizeof(float));

                return res;
            }

            void Surface::fill_poly(const Color & c, const float *x, const float *y, size_t n)
            {
                if (!pSurface->is_drawing())
                    return;

                float * coords   = copy_coords(x, y, n);
                if (coords == NULL)
                    return;
                lsp_finally {
                    if (coords != NULL)
                        free(coords);
                };

                gl::actions::draw_poly_t * const cmd = pSurface->append_command<gl::actions::draw_poly_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->data   = release_ptr(coords);
                cmd->width  = 0.0f;
                cmd->count  = n;
            }

            void Surface::fill_poly(IGradient *g, const float *x, const float *y, size_t n)
            {
                if (n < 3)
                    return;

                if (!pSurface->is_drawing())
                    return;

                float * coords   = copy_coords(x, y, n);
                if (coords == NULL)
                    return;
                lsp_finally {
                    if (coords != NULL)
                        free(coords);
                };

                gl::actions::draw_poly_t * const cmd = pSurface->append_command<gl::actions::draw_poly_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->data   = release_ptr(coords);
                cmd->width  = 0.0f;
                cmd->count  = n;
            }

            void Surface::wire_poly(const Color & c, float width, const float *x, const float *y, size_t n)
            {
                if ((width < 1e-6f) || (n < 2))
                    return;

                if (!pSurface->is_drawing())
                    return;

                float * coords   = copy_coords(x, y, n);
                if (coords == NULL)
                    return;
                lsp_finally {
                    if (coords != NULL)
                        free(coords);
                };

                gl::actions::draw_poly_t * const cmd = pSurface->append_command<gl::actions::draw_poly_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->wire, c);
                cmd->data   = release_ptr(coords);
                cmd->width  = width;
                cmd->count  = n;
            }

            void Surface::draw_poly(const Color &fill, const Color &wire, float width, const float *x, const float *y, size_t n)
            {
                if (n < 2)
                    return;

                if (!pSurface->is_drawing())
                    return;

                float * coords   = copy_coords(x, y, n);
                if (coords == NULL)
                    return;
                lsp_finally {
                    if (coords != NULL)
                        free(coords);
                };

                gl::actions::draw_poly_t * const cmd = pSurface->append_command<gl::actions::draw_poly_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, fill);
                set_fill(cmd->wire, wire);
                cmd->data   = release_ptr(coords);
                cmd->width  = width;
                cmd->count  = n;
            }

            void Surface::fill_circle(const Color & c, float x, float y, float r)
            {
                if (r <= 0.0f)
                    return;

                gl::actions::fill_circle_t * const cmd = pSurface->append_command<gl::actions::fill_circle_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->center_x   = x;
                cmd->center_y   = y;
                cmd->radius     = r;
            }

            void Surface::fill_circle(IGradient *g, float x, float y, float r)
            {
                if (r <= 0.0f)
                    return;

                gl::actions::fill_circle_t * const cmd = pSurface->append_command<gl::actions::fill_circle_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, g);
                cmd->center_x   = x;
                cmd->center_y   = y;
                cmd->radius     = r;
            }

            void Surface::fill_frame(
                const Color &c,
                size_t flags, float radius,
                float fx, float fy, float fw, float fh,
                float ix, float iy, float iw, float ih)
            {
                gl::actions::fill_frame_t * const cmd = pSurface->append_command<gl::actions::fill_frame_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->outer_rect.x       = fx;
                cmd->outer_rect.y       = fy;
                cmd->outer_rect.width   = fw;
                cmd->outer_rect.height  = fh;
                cmd->inner_rect.x       = ix;
                cmd->inner_rect.y       = iy;
                cmd->inner_rect.width   = iw;
                cmd->inner_rect.height  = ih;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(flags);
            }

            void Surface::fill_frame(
                const Color &c,
                size_t flags, float radius,
                const ws::rectangle_t *out, const ws::rectangle_t *in)
            {
                gl::actions::fill_frame_t * const cmd = pSurface->append_command<gl::actions::fill_frame_t>();
                if (cmd == NULL)
                    return;

                set_fill(cmd->fill, c);
                cmd->outer_rect.x       = out->nLeft;
                cmd->outer_rect.y       = out->nTop;
                cmd->outer_rect.width   = out->nWidth;
                cmd->outer_rect.height  = out->nHeight;
                cmd->inner_rect.x       = in->nLeft;
                cmd->inner_rect.y       = in->nTop;
                cmd->inner_rect.width   = in->nWidth;
                cmd->inner_rect.height  = in->nHeight;
                cmd->radius             = radius;
                cmd->corners            = uint32_t(flags);
            }

            bool Surface::get_antialiasing()
            {
                return bAntiAliasing;
            }

            bool Surface::set_antialiasing(bool set)
            {
                const bool old      = bAntiAliasing;
                bAntiAliasing       = set;

                gl::actions::set_antialiasing_t * const cmd = pSurface->append_command<gl::actions::set_antialiasing_t>();
                if (cmd != NULL)
                    cmd->enable         = set;

                return old;
            }

            ws::point_t Surface::set_origin(const ws::point_t & origin)
            {
                ws::point_t result;
                result.nLeft        = sOrigin.left;
                result.nTop         = sOrigin.top;

                sOrigin.left        = int32_t(origin.nLeft);
                sOrigin.top         = int32_t(origin.nTop);

                gl::actions::set_origin_t * const cmd = pSurface->append_command<gl::actions::set_origin_t>();
                if (cmd != NULL)
                    cmd->origin         = sOrigin;

                return result;
            }

            ws::point_t Surface::set_origin(ssize_t left, ssize_t top)
            {
                ws::point_t result;
                result.nLeft        = sOrigin.left;
                result.nTop         = sOrigin.top;

                sOrigin.left        = int32_t(left);
                sOrigin.top         = int32_t(top);

                gl::actions::set_origin_t * const cmd = pSurface->append_command<gl::actions::set_origin_t>();
                if (cmd != NULL)
                    cmd->origin         = sOrigin;

                return result;
            }

            void Surface::clip_begin(float x, float y, float w, float h)
            {
                gl::actions::clip_begin_t * const cmd = pSurface->append_command<gl::actions::clip_begin_t>();
                if (cmd == NULL)
                    return;

                cmd->rect.left      = x;
                cmd->rect.top       = y;
                cmd->rect.right     = x + w;
                cmd->rect.bottom    = y + h;

                if (sClipping.count >= gl::clip_state_t::MAX_CLIPS)
                {
                    lsp_error("Too many clipping regions specified (%d)", int(gl::clip_state_t::MAX_CLIPS + 1));
                    return;
                }

                sClipping.clips[sClipping.count++] = cmd->rect;
            }

            void Surface::clip_end()
            {
                gl::actions::clip_end_t * const cmd = pSurface->append_command<gl::actions::clip_end_t>();
                if (cmd == NULL)
                    return;

                if (sClipping.count <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return;
                }
                --sClipping.count;
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const char *text)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set_utf8(text))
                    return;

                gl::actions::out_text_t *cmd    = pSurface->append_command<gl::actions::out_text_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x      = x;
                cmd->y      = y;
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text))
                    return;

                gl::actions::out_text_t *cmd    = pSurface->append_command<gl::actions::out_text_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x      = x;
                cmd->y      = y;
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text, first))
                    return;

                gl::actions::out_text_t *cmd    = pSurface->append_command<gl::actions::out_text_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x      = x;
                cmd->y      = y;
            }

            void Surface::out_text(const Font &f, const Color &color, float x, float y, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text, first, last))
                    return;

                gl::actions::out_text_t *cmd    = pSurface->append_command<gl::actions::out_text_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x      = x;
                cmd->y      = y;
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const char *text)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set_utf8(text))
                    return;

                gl::actions::out_text_relative_t *cmd       = pSurface->append_command<gl::actions::out_text_relative_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x          = x;
                cmd->y          = y;
                cmd->relative_x = dx;
                cmd->relative_y = dy;
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text))
                    return;

                gl::actions::out_text_relative_t *cmd       = pSurface->append_command<gl::actions::out_text_relative_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x          = x;
                cmd->y          = y;
                cmd->relative_x = dx;
                cmd->relative_y = dy;
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text, first))
                    return;

                gl::actions::out_text_relative_t *cmd       = pSurface->append_command<gl::actions::out_text_relative_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x          = x;
                cmd->y          = y;
                cmd->relative_x = dx;
                cmd->relative_y = dy;
            }

            void Surface::out_text_relative(const Font &f, const Color &color, float x, float y, float dx, float dy, const LSPString *text, ssize_t first, ssize_t last)
            {
                if ((f.get_name() == NULL) || (text == NULL))
                    return;

                LSPString tmp_text;
                if (!tmp_text.set(text, first, last))
                    return;

                gl::actions::out_text_relative_t *cmd       = pSurface->append_command<gl::actions::out_text_relative_t>();
                if (cmd == NULL)
                    return;

                set_color(cmd->fill, color);
                cmd->font.set(f);
                cmd->text.swap(&tmp_text);
                cmd->x          = x;
                cmd->y          = y;
                cmd->relative_x = dx;
                cmd->relative_y = dy;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

