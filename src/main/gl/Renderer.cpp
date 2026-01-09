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

#include <private/gl/Renderer.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/ws/ISurface.h>

#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            // ----------------------------------------------------------------------------
            static constexpr float MATH_PI = M_PI;
            static constexpr float MATH_PI_MUL_1D4  = MATH_PI * 0.25f;
            static constexpr float MATH_PI_MUL_1D2  = MATH_PI * 0.5f;
            static constexpr float MATH_PI_MUL_3D2  = MATH_PI * 1.5f;
            static constexpr float MATH_PI_MUL_2    = MATH_PI * 2.0f;

            #define ADD_TVERTEX(v, v_ci, v_x, v_y, v_s, v_t) \
                v->x        = v_x; \
                v->y        = v_y; \
                v->s        = v_s; \
                v->t        = v_t; \
                v->cmd      = v_ci; \
                ++v;

            #define ADD_VERTEX(v, v_ci, v_x, v_y)  ADD_TVERTEX(v, v_ci, v_x, v_y, 0.0f, 0.0f)

            #define ADD_HRECTANGLE(v, a, b, c, d) \
                v[0]        = a; \
                v[1]        = b; \
                v[2]        = c; \
                v[3]        = a; \
                v[4]        = c; \
                v[5]        = d; \
                v += 6;

            #define ADD_HTRIANGLE(v, a, b, c) \
                v[0]        = a; \
                v[1]        = b; \
                v[2]        = c; \
                v += 3;

            constexpr float k_color = 1.0f / 255.0f;

            // ----------------------------------------------------------------------------
            Renderer::Renderer(gl::IContext * gl_context):
                pGLContext(safe_acquire(gl_context)),
                sThread(execute, this),
                sTextAllocator(pGLContext),
                sBatch(&sAllocator)
            {
                atomic_store(&nReferences, 1);
            }

            Renderer::~Renderer()
            {
                do_destroy();
            }

            status_t Renderer::init()
            {
                status_t res = sBatch.init();
                if (res != STATUS_OK)
                    return res;

                return sThread.start();
            }

            void Renderer::destroy()
            {
                do_destroy();
            }

            void Renderer::do_destroy()
            {
                // Terminate the thread
                {
                    sLock.lock();
                    lsp_finally { sLock.unlock(); };

                    sThread.cancel();
                    sLock.notify();
                }

                // Wait until thread has terminated
                sThread.join();

                // Release context and texture allocator
                safe_release(pGLContext);
            }

            uatomic_t Renderer::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t Renderer::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                {
                    destroy();
                    delete this;
                }
                return result;
            }

            status_t Renderer::execute(void *arg)
            {
                dsp::context_t ctx;
                dsp::start(&ctx);
                lsp_finally { dsp::finish(&ctx); };

                Renderer * const self = static_cast<Renderer *>(arg);
                return self->run();
            }

            SurfaceContext *Renderer::poll()
            {
                sLock.lock();
                lsp_finally { sLock.unlock(); };

                // Wait until new event occurs
                while (true)
                {
                    if (sThread.cancelled())
                        return NULL;

                    if (!sQueue.is_empty())
                        return safe_acquire(sQueue.first());

                    sLock.wait();
                }
            }

            status_t Renderer::queue_draw(SurfaceContext * surface)
            {
                sLock.lock();
                lsp_finally { sLock.unlock(); };

                if (!sQueue.append(surface))
                    return STATUS_NO_MEM;

                surface->reference_up();
                surface->begin_render();
                sLock.notify();

                return STATUS_OK;
            }

            bool Renderer::update_uniforms(lltl::darray<gl::uniform_t> & uniforms, SurfaceContext * surface)
            {
                uniforms.clear();
                gl::uniform_t * const model = uniforms.add();
                if (model == NULL)
                    return false;

                gl::uniform_t * const end   = uniforms.add();
                if (end == NULL)
                    return false;

                model->name     = "u_model";
                model->type     = gl::UNI_MAT4F;
                model->f32      = reinterpret_cast<const GLfloat *>(surface->matrix().v);

                end->name       = NULL;
                end->type       = gl::UNI_NONE;
                end->raw        = NULL;

                return true;
            }

            gl::Texture *Renderer::make_text(texture_rect_t *rect, const void *data, size_t width, size_t height, size_t stride)
            {
                // Check that texture can be placed to the atlas
                gl::Texture *tex = NULL;
                if ((width <= TEXT_ATLAS_SIZE) && (height <= TEXT_ATLAS_SIZE))
                {
                    ws::rectangle_t wrect;

                    tex = sTextAllocator.allocate(&wrect, data, width, height, stride);
                    if (tex != NULL)
                    {
                        rect->sb        = wrect.nLeft * gl::TEXT_ATLAS_SCALE;
                        rect->tb        = wrect.nTop * gl::TEXT_ATLAS_SCALE;
                        rect->se        = (wrect.nLeft + wrect.nWidth) * gl::TEXT_ATLAS_SCALE;
                        rect->te        = (wrect.nTop + wrect.nHeight) * gl::TEXT_ATLAS_SCALE;

                        return tex;
                    }
                }

                // Allocate texture
                tex = new gl::Texture(pGLContext);
                if (tex == NULL)
                    return NULL;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                if (tex->set_image(data, width, height, stride, gl::TEXTURE_ALPHA8) != STATUS_OK)
                    return NULL;

                rect->sb        = 0.0f;
                rect->tb        = 0.0f;
                rect->se        = 1.0f;
                rect->te        = 1.0f;

                return release_ptr(tex);
            }

            status_t Renderer::setup_context(SurfaceContext * surface)
            {
                // Skip invalid surfaces
                if (!surface->valid())
                    return STATUS_SKIP;

                // Activate OpenGL context
                return pGLContext->activate(surface->drawable());
            }

            status_t Renderer::run()
            {
                lltl::darray<gl::uniform_t> uniforms;
                SurfaceContext * surface = NULL;
                status_t res;

                // Do a loop while there are jobs to do
                while ((surface = poll()) != NULL)
                {
                    // Remove context from the queue and release it on the end of loop
                    lsp_finally
                    {
                        sLock.lock();
                        sQueue.shift();
                        surface->end_render();
                        sLock.unlock();

                        safe_release(surface);
                    };

                    // Set up OpenGL context for drawing
                    res = setup_context(surface);
                    if (res != STATUS_OK)
                        continue;

                    // Notify context about start of the rendering
                    lsp_finally {
                        sBatch.clear();
                        pGLContext->deactivate();
                        sAllocator.perform_gc();
                        sTextAllocator.clear();
                    };

                    // Process each action in the list
                    res         = STATUS_OK;
                    for (const gl::actions::action_t * action = surface->current_action();
                        action != NULL;
                        action = surface->next_action())
                    {
                        if ((res = process(surface, *action)) != STATUS_OK)
                            break;
                    }

                    // Execute batch
                    if (res == STATUS_OK)
                    {
                        if (update_uniforms(uniforms, surface))
                            sBatch.execute(pGLContext, uniforms.array());

                        // Swap buffers for non-nested surfaces
                        if ((surface->valid()) && (!surface->is_nested()))
                            pGLContext->swap_buffers(surface->width(), surface->height());
                    }
                    else
                        lsp_trace("Render failed with error code=%d", int(res));
                }

                // Destroy context
                pGLContext->destroy();

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const gl::actions::action_t & action)
            {
                #define PROC(enum, field) \
                    case actions::enum: return process(surface, action.field);

                switch (action.type)
                {
                    PROC(INIT, init);
                    PROC(CLEAR, clear);
                    PROC(RESIZE, resize);
                    PROC(DRAW_SURFACE, draw_surface);
                    PROC(DRAW_RAW, draw_raw);
                    PROC(WIRE_RECT, wire_rect);
                    PROC(FILL_RECT, fill_rect);
                    PROC(FILL_SECTOR, fill_sector);
                    PROC(FILL_TRIANGLE, fill_triangle);
                    PROC(FILL_CIRCLE, fill_circle);
                    PROC(WIRE_ARC, wire_arc);
                    PROC(OUT_TEXT, out_text);
                    PROC(OUT_TEXT_BITMAP, out_text_bitmap);
                    PROC(OUT_TEXT_RELATIVE, out_text_relative);
                    PROC(LINE, line);
                    PROC(PARAMETRIC_LINE, parametric_line);
                    PROC(PARAMETRIC_BAR, parametric_bar);
                    PROC(FILL_FRAME, fill_frame);
                    PROC(DRAW_POLY, draw_poly);
                    PROC(CLIP_BEGIN, clip_begin);
                    PROC(CLIP_END, clip_end);
                    PROC(SET_ANTIALIASING, set_antialiasing);
                    PROC(SET_ORIGIN, set_origin);
                    default:
                        break;
                }

                #undef PROC

                return STATUS_INVALID_VALUE;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::init_t & action)
            {
                surface->set_size(action.size);
                surface->origin()  = action.origin;
                surface->set_antialiasing(action.antialiasing);

                // Perform rendering of the collected batch
                const gl::vtbl_t *vtbl = pGLContext->vtbl();

                if (surface->is_nested())
                {
                    // Ensure that texture is properly initialized
                    gl::Texture *texture = surface->texture();
                    if (texture == NULL)
                    {
                        texture = new gl::Texture(pGLContext);
                        if (texture == NULL)
                            return STATUS_NO_MEM;
                        surface->set_texture(texture);
                    }
                    lsp_finally { safe_release(texture); };

                    // Setup texture for drawing
                    status_t res = texture->begin_draw(action.size.width, action.size.height, gl::TEXTURE_PRGBA32);
                    if (res != STATUS_OK)
                        return res;
                    lsp_finally { texture->end_draw(); };

                    vtbl->glViewport(0, 0, action.size.width, action.size.height);
                }
                else
                {
                    // Setup viewport
                    const ssize_t height = pGLContext->height();
                    vtbl->glViewport(0, height - action.size.height, action.size.width, action.size.height);

                    // Set target drawing buffer
                    vtbl->glDrawBuffer(GL_BACK);
                }

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::clear_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_NO_BLENDING, action.color);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_rect(uint32_t(res), 0.0f, 0.0f, surface->width(), surface->height());
                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::resize_t & action)
            {
                surface->set_size(action.size);
                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::draw_surface_t & action)
            {
                // Start batch
                gl::Texture * const t = action.fill.surface->texture();
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, t, action.fill.blend);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return STATUS_NO_MEM;

                if (fabsf(action.angle < 1e-6f))
                {
                    const uint32_t ci   = uint32_t(res);
                    const float xe      = action.x + t->width() * action.scale_x;
                    const float ye      = action.y + t->height() * action.scale_y;

                    ADD_TVERTEX(v, ci, action.x, action.y, 0.0f, 1.0f);
                    ADD_TVERTEX(v, ci, action.x, ye, 0.0f, 0.0f);
                    ADD_TVERTEX(v, ci, xe, ye, 1.0f, 0.0f);
                    ADD_TVERTEX(v, ci, xe, action.y, 1.0f, 1.0f);
                }
                else
                {
                    const uint32_t ci   = uint32_t(res);
                    const float ca      = cosf(action.angle);
                    const float sa      = sinf(action.angle);

                    const float sx      = action.scale_x * t->width();
                    const float sy      = action.scale_y * t->height();

                    const float v1x     = ca * sx;
                    const float v1y     = sa * sx;
                    const float v2x     = -sa * sy;
                    const float v2y     = ca * sy;

                    ADD_TVERTEX(v, ci, action.x, action.y, 0.0f, 1.0f);
                    ADD_TVERTEX(v, ci, action.x + v2x, action.y + v2y, 0.0f, 0.0f);
                    ADD_TVERTEX(v, ci, action.x + v1x + v2x, action.y + v1y + v2y, 1.0f, 0.0f);
                    ADD_TVERTEX(v, ci, action.x + v1x, action.y + v1y, 1.0f, 1.0f);
                }

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::draw_raw_t & action)
            {
                gl::Texture *tex = new gl::Texture(pGLContext);
                if (tex == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { safe_release(tex); };

                // Initialize texture
                ssize_t res         = tex->set_image(action.data, action.width, action.height, action.stride, TEXTURE_PRGBA32);
                if (res != STATUS_OK)
                    return status_t(res);

                // Start batch
                res                 = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, tex, action.blend);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw primitives
                const uint32_t ci   = uint32_t(res);
                const float xe      = action.x + action.width * action.scale_x;
                const float ye      = action.y + action.height * action.scale_y;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return STATUS_NO_MEM;

                ADD_TVERTEX(v, ci, action.x, action.y, 0.0f, 0.0f);
                ADD_TVERTEX(v, ci, action.x, ye, 0.0f, 1.0f);
                ADD_TVERTEX(v, ci, xe, ye, 1.0f, 1.0f);
                ADD_TVERTEX(v, ci, xe, action.y, 1.0f, 0.0f);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::wire_rect_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };


                // Draw primitives
                const uint32_t ci   = uint32_t(res);
                const float left    = action.rectangle.x;
                const float top     = action.rectangle.y;
                const float right   = left + action.rectangle.width;
                const float bottom  = top + action.rectangle.height;
                const float line_width  = action.line_width;
                const float radius  = action.radius;
                const float xr      = radius - line_width * 0.5f;

                // Bounds of horizontal and vertical rectangles
                float top_l         = left;
                float top_r         = right;
                float bot_l         = top_l;
                float bot_r         = top_r;
                float lef_t         = top + line_width;
                float lef_b         = bottom - line_width;
                float rig_t         = lef_t;
                float rig_b         = lef_b;

                if (action.corners & SURFMASK_LT_CORNER)
                {
                    top_l           = left + radius;
                    lef_t           = top + radius;
                    wire_arc(ci, top_l, lef_t, xr, MATH_PI, MATH_PI_MUL_3D2, line_width);
                }
                if (action.corners & SURFMASK_RT_CORNER)
                {
                    top_r           = right - radius;
                    rig_t           = top + radius;
                    wire_arc(ci, top_r, rig_t, xr, MATH_PI_MUL_3D2, MATH_PI_MUL_2, line_width);
                }
                if (action.corners & SURFMASK_LB_CORNER)
                {
                    bot_l           = left + radius;
                    lef_b           = bottom - radius;
                    wire_arc(ci, bot_l, lef_b, xr, MATH_PI_MUL_1D2, MATH_PI, line_width);
                }
                if (action.corners & SURFMASK_RB_CORNER)
                {
                    bot_r           = right - radius;
                    rig_b           = bottom - radius;
                    wire_arc(ci, bot_r, rig_b, xr, 0.0f, MATH_PI_MUL_1D2, line_width);
                }

                fill_rect(ci, top_l, top, top_r, top + line_width);
                fill_rect(ci, bot_l, bottom - line_width, bot_r, bottom);
                fill_rect(ci, left, lef_t, left + line_width, lef_b);
                fill_rect(ci, right - line_width, rig_t, right, rig_b);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::fill_rect_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                const uint32_t ci   = uint32_t(res);
                float left          = action.rectangle.x;
                float top           = action.rectangle.y;
                float right         = left + action.rectangle.width;
                float bottom        = top + action.rectangle.height;
                const float radius  = action.radius;

                // Draw primitives
                if (action.fill.type == FILL_TEXTURE)
                {
                    texcoord_t tex;
                    tex.x       = action.rectangle.x;
                    tex.y       = action.rectangle.y + action.rectangle.height;
                    tex.sx      = 1.0f / action.rectangle.width;
                    tex.sy      = -1.0f / action.rectangle.height;

                    if (action.corners & SURFMASK_T_CORNER)
                    {
                        float l         = left;
                        float r         = right;
                        top            += radius;

                        if (action.corners & SURFMASK_LT_CORNER)
                        {
                            l              += radius;
                            fill_sector_textured(ci, tex, l, top, radius, MATH_PI, MATH_PI_MUL_3D2);
                        }
                        if (action.corners & SURFMASK_RT_CORNER)
                        {
                            r              -= radius;
                            fill_sector_textured(ci, tex, r, top, radius, MATH_PI_MUL_3D2, MATH_PI_MUL_2);
                        }
                        fill_rect_textured(ci, tex, l, top - radius, r, top);
                    }
                    if (action.corners & SURFMASK_B_CORNER)
                    {
                        float l         = left;
                        float r         = right;
                        bottom         -= radius;

                        if (action.corners & SURFMASK_LB_CORNER)
                        {
                            l              += radius;
                            fill_sector_textured(ci, tex, l, bottom, radius, MATH_PI_MUL_1D2, MATH_PI);
                        }
                        if (action.corners & SURFMASK_RB_CORNER)
                        {
                            r              -= radius;
                            fill_sector_textured(ci, tex, r, bottom, radius, 0.0f, MATH_PI_MUL_1D2);
                        }
                        fill_rect_textured(ci, tex, l, bottom, r, bottom + radius);
                    }

                    fill_rect_textured(ci, tex, left, top, right, bottom);
                }
                else
                {
                    // Draw primitives
                    if (action.corners & SURFMASK_T_CORNER)
                    {
                        float l         = left;
                        float r         = right;
                        top            += radius;

                        if (action.corners & SURFMASK_LT_CORNER)
                        {
                            l              += radius;
                            fill_sector(ci, l, top, radius, MATH_PI, MATH_PI_MUL_3D2);
                        }
                        if (action.corners & SURFMASK_RT_CORNER)
                        {
                            r              -= radius;
                            fill_sector(ci, r, top, radius, MATH_PI_MUL_3D2, MATH_PI_MUL_2);
                        }
                        fill_rect(ci, l, top - radius, r, top);
                    }
                    if (action.corners & SURFMASK_B_CORNER)
                    {
                        float l         = left;
                        float r         = right;
                        bottom         -= radius;

                        if (action.corners & SURFMASK_LB_CORNER)
                        {
                            l              += radius;
                            fill_sector(ci, l, bottom, radius, MATH_PI_MUL_1D2, MATH_PI);
                        }
                        if (action.corners & SURFMASK_RB_CORNER)
                        {
                            r              -= radius;
                            fill_sector(ci, r, bottom, radius, 0.0f, MATH_PI_MUL_1D2);
                        }
                        fill_rect(ci, l, bottom, r, bottom + radius);
                    }

                    fill_rect(ci, left, top, right, bottom);
                }

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::fill_sector_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_sector(uint32_t(res), action.center_x, action.center_y, action.radius, action.angle_start, action.angle_end);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::fill_triangle_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_triangle(uint32_t(res),
                    action.x[0], action.y[0],
                    action.x[1], action.y[1],
                    action.x[2], action.y[2]);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::fill_circle_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                fill_circle(uint32_t(res), action.center_x, action.center_y, action.radius);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::wire_arc_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                wire_arc(uint32_t(res),
                    action.center_x, action.center_y,
                    action.radius, action.angle_start, action.angle_end,
                    action.width);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::out_text_t & action)
            {
                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::out_text_bitmap_t & action)
            {
                // Allocate texture
                const dsp::bitmap_t *bitmap = action.bitmap;

                texture_rect_t rect;
                gl::Texture *tex        = make_text(&rect, bitmap->data, bitmap->width, bitmap->height, bitmap->stride);
                if (tex == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { safe_release(tex); };

                // Output the text
                {
                    const ssize_t res   = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, tex, action.fill);
                    if (res < 0)
                        return status_t(-res);
                    lsp_finally { sBatch.end(); };

                    // Draw primitives
                    const uint32_t ci   = uint32_t(res);
                    const float xs      = action.x;
                    const float ys      = action.y;
                    const float xe      = xs + bitmap->width;
                    const float ye      = ys + bitmap->height;

                    const uint32_t vi   = sBatch.next_vertex_index();
                    gl::vertex_t *v     = sBatch.add_vertices(4);
                    if (v == NULL)
                        return STATUS_NO_MEM;

                    ADD_TVERTEX(v, ci, xs, ys, rect.sb, rect.tb);
                    ADD_TVERTEX(v, ci, xs, ye, rect.sb, rect.te);
                    ADD_TVERTEX(v, ci, xe, ye, rect.se, rect.te);
                    ADD_TVERTEX(v, ci, xe, ys, rect.se, rect.tb);

                    sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
                }

                // Draw underline if required
                if ((action.underline.width > 1e-6f) && (action.underline.height > 1e-6f))
                {
                    const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                    if (res < 0)
                        return status_t(-res);
                    lsp_finally { sBatch.end(); };

                    fill_rect(uint32_t(res),
                        action.underline.x, action.underline.y,
                        action.underline.x + action.underline.width, action.underline.y + action.underline.height);
                }

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::out_text_relative_t & action)
            {
                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::line_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                wire_line(uint32_t(res),
                    action.x[0], action.y[0],
                    action.x[1],
                    action.y[1],
                    action.width);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::parametric_line_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw the line
                if (fabsf(action.a) > fabsf(action.b))
                {
                    const float k = -1.0f / action.a;
                    wire_line(uint32_t(res),
                        roundf((action.c + action.b*action.rect.top) * k), roundf(action.rect.top),
                        roundf((action.c + action.b*action.rect.bottom) * k), roundf(action.rect.bottom),
                        action.width);
                }
                else
                {
                    const float k = -1.0f / action.b;
                    wire_line(uint32_t(res),
                        roundf(action.rect.left), roundf((action.c + action.a*action.rect.left) * k),
                        roundf(action.rect.right), roundf((action.c + action.a*action.rect.right) * k),
                        action.width);
                }

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::parametric_bar_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw the primitive
                const uint32_t ci   = uint32_t(res);
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return STATUS_NO_MEM;

                if (fabsf(action.a[0]) > fabsf(action.b[0]))
                {
                    const float k = -1.0f / action.a[0];
                    ADD_VERTEX(v, ci, (action.c[0] + action.b[0]*action.rect.top)*k, action.rect.top);
                    ADD_VERTEX(v, ci, (action.c[0] + action.b[0]*action.rect.bottom)*k, action.rect.bottom);
                }
                else
                {
                    const float k = -1.0f / action.b[0];
                    ADD_VERTEX(v, ci, action.rect.left, (action.c[0] + action.a[0]*action.rect.left)*k);
                    ADD_VERTEX(v, ci, action.rect.right, (action.c[0] + action.a[0]*action.rect.right)*k);
                }

                if (fabsf(action.a[1]) > fabsf(action.b[1]))
                {
                    const float k = -1.0f / action.a[1];
                    ADD_VERTEX(v, ci, (action.c[1] + action.b[1]*action.rect.bottom)*k, action.rect.bottom);
                    ADD_VERTEX(v, ci, (action.c[1] + action.b[1]*action.rect.top)*k, action.rect.top);
                }
                else
                {
                    const float k = -1.0f / action.b[1];
                    ADD_VERTEX(v, ci, action.rect.right, (action.c[1] + action.a[1]*action.rect.right)*k);
                    ADD_VERTEX(v, ci, action.rect.left, (action.c[1] + action.a[1]*action.rect.left)*k);
                }

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::fill_frame_t & action)
            {
                // Start batch
                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                if (res < 0)
                    return status_t(-res);
                lsp_finally { sBatch.end(); };

                // Draw geometry
                const uint32_t ci   = uint32_t(res);
                const float fx      = action.outer_rect.x;
                const float fy      = action.outer_rect.y;
                const float ix      = action.inner_rect.x;
                const float iy      = action.inner_rect.y;
                const float r       = action.radius;
                const float fxe     = fx + action.outer_rect.width;
                const float fye     = fy + action.outer_rect.height;
                const float ixe     = ix + action.inner_rect.width;
                const float iye     = iy + action.inner_rect.height;

                // Simple case
                if ((ix >= fxe) || (ixe < fx) || (iy >= fye) || (iye < fy))
                {
                    fill_rect(ci, fx, fy, fxe, fye);
                    return STATUS_OK;
                }
                else if ((ix <= fx) && (ixe >= fxe) && (iy <= fy) && (iye >= fye))
                    return STATUS_OK;

                if (fy < iy) // Top rectangle
                    fill_rect(ci, fx, fy, fxe, iy);
                if (fye > iye) // Bottom rectangle
                    fill_rect(ci, fx, iye, fxe, fye);

                const float vt  = lsp_max(fy, iy);
                const float vb  = lsp_min(fye, iye);
                if (fx < ix) // Left rectangle
                    fill_rect(ci, fx, vt, ix, vb);
                if (fxe > ixe) // Right rectangle
                    fill_rect(ci, ixe, vt, fxe, vb);

                if (action.corners & SURFMASK_LT_CORNER)
                    fill_corner(ci, ix + r, iy + r, ix, iy, r, MATH_PI);
                if (action.corners & SURFMASK_RT_CORNER)
                    fill_corner(ci, ixe - r, iy + r, ixe, iy, r, MATH_PI_MUL_3D2);
                if (action.corners & SURFMASK_LB_CORNER)
                    fill_corner(ci, ix + r, iye - r, ix, iye, r, MATH_PI_MUL_1D2);
                if (action.corners & SURFMASK_RB_CORNER)
                    fill_corner(ci, ixe - r, iye - r, ixe, iye, r, 0.0f);

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::draw_poly_t & action)
            {
                static const gl::color_t empty_color = { 0.0f, 0.0f, 0.0f, 0.0f };

                const float * const x = action.data;
                const float * const y = &action.data[action.count];

                // Fill poly if needed
                if ((action.fill.type != FILL_NONE) && (action.count >= 3))
                {
                    if (action.count > 3)
                    {
                        // Start first batch on stencil buffer
                        clip_rect_t rect;
                        {
                            const ssize_t res = start_batch(surface, gl::STENCIL, gl::BATCH_STENCIL_OP_XOR | gl::BATCH_CLEAR_STENCIL, empty_color);
                            if (res < 0)
                                return status_t(-res);
                            lsp_finally{ sBatch.end(); };

                            fill_triangle_fan(size_t(res), rect, x, y, action.count);
                            limit_rect(rect, surface);
                        }

                        // Start second batch on color buffer with stencil apply
                        {
                            const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_STENCIL_OP_APPLY, action.fill);
                            if (res < 0)
                                return status_t(-res);
                            lsp_finally{ sBatch.end(); };

                            fill_rect(size_t(res), rect.left, rect.top, rect.right, rect.bottom);
                        }
                    }
                    else
                    {
                        // Some optimizations
                        const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.fill);
                        if (res < 0)
                            return status_t(-res);
                        lsp_finally { sBatch.end(); };

                        // Draw geometry
                        fill_triangle(uint32_t(res), x[0], y[0], x[1], y[1], x[2], y[2]);
                    }
                }

                // Wire poly if needed
                if ((action.wire.type != FILL_NONE) && (action.width >= 1e-6f) && (action.count >= 2))
                {
                    if (action.count > 2)
                    {
                        if ((action.wire.type == FILL_SOLID_COLOR) && (action.wire.color.a < k_color))
                        {
                            // Opaque polyline can be drawin without stencil buffer
                            const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.wire);
                            if (res < 0)
                                return status_t(-res);
                            lsp_finally{ sBatch.end(); };

                            wire_polyline(size_t(res), x, y, action.width, action.count);
                        }
                        else
                        {
                            // Start first batch on stencil buffer
                            clip_rect_t rect;
                            {
                                rect.left       = surface->width();
                                rect.top        = surface->height();
                                rect.right      = 0.0f;
                                rect.bottom     = 0.0f;

                                const ssize_t res = start_batch(surface, gl::STENCIL, gl::BATCH_STENCIL_OP_OR | gl::BATCH_CLEAR_STENCIL, empty_color);
                                if (res < 0)
                                    return status_t(-res);
                                lsp_finally{ sBatch.end(); };

                                wire_polyline(size_t(res), rect, x, y, action.width, action.count);
                                limit_rect(rect, surface);
                            }

                            // Start second batch on color buffer with stencil apply
                            {
                                const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR | gl::BATCH_STENCIL_OP_APPLY, action.fill);
                                if (res < 0)
                                    return status_t(-res);
                                lsp_finally{ sBatch.end(); };

                                fill_rect(size_t(res), rect.left, rect.top, rect.right, rect.bottom);
                            }
                        }
                    }
                    else
                    {
                        // Draw simple line
                        const ssize_t res = start_batch(surface, gl::GEOMETRY, gl::BATCH_WRITE_COLOR, action.wire);
                        if (res < 0)
                            return status_t(-res);
                        lsp_finally { sBatch.end(); };

                        // Draw geometry
                        wire_line(uint32_t(res), x[0], y[0], x[1], y[1], action.width);
                    }
                }

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::clip_begin_t & action)
            {
                gl::clip_state_t & clipping = surface->clipping();
                if (clipping.count >= gl::clip_state_t::MAX_CLIPS)
                {
                    lsp_error("Too many clipping regions specified (%d)", int(gl::clip_state_t::MAX_CLIPS + 1));
                    return STATUS_OVERFLOW;
                }

                clipping.clips[clipping.count++] = action.rect;

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::clip_end_t & action)
            {
                gl::clip_state_t & clipping = surface->clipping();
                if (clipping.count <= 0)
                {
                    lsp_error("Mismatched number of clip_begin() and clip_end() calls");
                    return STATUS_UNDERFLOW;
                }
                --clipping.count;

                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::set_antialiasing_t & action)
            {
                surface->set_antialiasing(action.enable);
                return STATUS_OK;
            }

            status_t Renderer::process(SurfaceContext * surface, const actions::set_origin_t & action)
            {
                surface->origin() = action.origin;
                return STATUS_OK;
            }

            inline float *Renderer::serialize_clipping(float *dst, const gl::clip_state_t & clipping)
            {
                for (size_t i=0; i<clipping.count; ++i)
                {
                    const gl::clip_rect_t & r = clipping.clips[i];
                    dst[0]          = r.left;
                    dst[1]          = r.top;
                    dst[2]          = r.right;
                    dst[3]          = r.bottom;

                    dst            += 4;
                }

                return dst;
            }

            inline float *Renderer::serialize_color(float *dst, const gl::color_t & c)
            {
                const float a   = 1.0f - c.a;
                dst[0]          = c.r * a;
                dst[1]          = c.g * a;
                dst[2]          = c.b * a;
                dst[3]          = a;

                return dst + 4;
            }

            inline void Renderer::extend_rect(clip_rect_t &rect, float x, float y)
            {
                rect.left           = lsp_min(rect.left, x);
                rect.top            = lsp_min(rect.top, y);
                rect.right          = lsp_max(rect.right, x);
                rect.bottom         = lsp_max(rect.bottom, y);
            }

            inline void Renderer::limit_rect(clip_rect_t & rect, SurfaceContext * surface)
            {
                const gl::origin_t & origin = surface->origin();
                rect.left           = lsp_max(rect.left, -origin.left);
                rect.top            = lsp_max(rect.top, -origin.top);
                rect.right          = lsp_min(rect.right, float(surface->width()) - origin.left);
                rect.bottom         = lsp_min(rect.bottom, float(surface->height()) - origin.top);
            }

            ssize_t Renderer::make_command(ssize_t index, cmd_color_t color, const gl::clip_state_t & clipping)
            {
                return (index << 5) | (size_t(color) << 3) | clipping.count;
            }

            ssize_t Renderer::start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::color_t & color)
            {
                // Start batch
                const gl::origin_t & origin = surface->origin();
                if (surface->antialiasing())
                    flags      |= BATCH_MULTISAMPLE;

                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        origin.left,
                        origin.top,
                        flags,
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                const gl::clip_state_t & clipping = surface->clipping();
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + clipping.count * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf, clipping);
                serialize_color(buf, color);

                return make_command(index, C_SOLID, clipping);
            }

            ssize_t Renderer::start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::linear_gradient_t & g)
            {
                // Start batch
                const gl::origin_t & origin = surface->origin();
                if (surface->antialiasing())
                    flags      |= BATCH_MULTISAMPLE;

                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        origin.left,
                        origin.top,
                        flags,
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                const size_t szof = 12 * sizeof(float);
                const gl::clip_state_t & clipping = surface->clipping();
                ssize_t index = sBatch.command(&buf, (szof + clipping.count * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                // Serialize clipping
                buf     = serialize_clipping(buf, clipping);

                // Serialize gradient
                buf     = serialize_color(buf, g.start);
                buf     = serialize_color(buf, g.end);
                buf[0]  = g.x1;
                buf[1]  = g.y1;
                buf[2]  = g.x2;
                buf[3]  = g.y2;

                return make_command(index, C_LINEAR, clipping);
            }

            ssize_t Renderer::start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::radial_gradient_t & g)
            {
                // Start batch
                const gl::origin_t & origin = surface->origin();
                if (surface->antialiasing())
                    flags      |= BATCH_MULTISAMPLE;

                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        origin.left,
                        origin.top,
                        flags,
                        NULL,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                const size_t szof = 16 * sizeof(float);
                const gl::clip_state_t & clipping = surface->clipping();
                ssize_t index = sBatch.command(&buf, (szof + clipping.count * sizeof(clip_rect_t)) / sizeof(float));
                if (index < 0)
                    return index;

                // Serialize clipping
                buf     = serialize_clipping(buf, clipping);

                // Serialize gradient
                buf     = serialize_color(buf, g.start);
                buf     = serialize_color(buf, g.end);
                buf[0]  = g.x1;
                buf[1]  = g.y1;
                buf[2]  = g.x2;
                buf[3]  = g.y2;
                buf[4]  = g.r;
                buf[5]  = 0.0f;
                buf[6]  = 0.0f;
                buf[7]  = 0.0f;

                return make_command(index, C_RADIAL, clipping);
            }

            ssize_t Renderer::start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, gl::Texture * const texture, const gl::color_t & color)
            {
                // Start batch
                if (texture == NULL)
                    return -STATUS_BAD_ARGUMENTS;
                const gl::origin_t & origin = surface->origin();
                if (surface->antialiasing())
                    flags      |= BATCH_MULTISAMPLE;

                status_t res = sBatch.begin(
                    gl::batch_header_t {
                        program,
                        origin.left,
                        origin.top,
                        flags,
                        texture,
                    });
                if (res != STATUS_OK)
                    return -res;

                // Allocate place for command
                float *buf = NULL;
                const gl::clip_state_t & clipping = surface->clipping();
                ssize_t index = sBatch.command(&buf, (sizeof(color_t) + clipping.count * sizeof(clip_rect_t) + 4 * sizeof(float)) / sizeof(float));
                if (index < 0)
                    return index;

                buf     = serialize_clipping(buf, clipping);
                buf     = serialize_color(buf, color);

                buf[0]  = float(texture->width());
                buf[1]  = float(texture->height());
                buf[2]  = texture->format();
                buf[3]  = texture->multisampling();

                return make_command(index, C_TEXTURE, clipping);
            }

            ssize_t Renderer::start_batch(SurfaceContext * surface, gl::program_t program, uint32_t flags, const gl::fill_t & fill)
            {
                switch (fill.type)
                {
                    case gl::FILL_SOLID_COLOR:
                        return start_batch(surface, program, flags, fill.color);
                    case gl::FILL_LINEAR_GRADIENT:
                        return start_batch(surface, program, flags, fill.linear);
                    case gl::FILL_RADIAL_GRADIENT:
                        return start_batch(surface, program, flags, fill.radial);
                    case gl::FILL_TEXTURE:
                        return start_batch(surface, program, flags, fill.texture.surface->texture(), fill.texture.blend);
                    default:
                        break;
                }

                return STATUS_INVALID_VALUE;
            }

            void Renderer::fill_triangle(uint32_t ci, float x0, float y0, float x1, float y1, float x2, float y2)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(3);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0, y0);
                ADD_VERTEX(v, ci, x1, y1);
                ADD_VERTEX(v, ci, x2, y2);

                sBatch.htriangle(vi, vi + 1, vi + 2);
            }

            void Renderer::fill_triangle_fan(uint32_t ci, clip_rect_t &rect, const float *x, const float *y, size_t n)
            {
                if (n < 3)
                    return;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(n);
                if (v == NULL)
                    return;

                // Fill geometry
                ADD_VERTEX(v, ci, x[0], y[0]);
                ADD_VERTEX(v, ci, x[1], y[1]);

                rect.left           = lsp_min(x[0], x[1]);
                rect.top            = lsp_min(y[0], y[1]);
                rect.right          = lsp_max(x[0], x[1]);
                rect.bottom         = lsp_max(y[0], y[1]);

                for (size_t i=2; i<n; ++i)
                {
                    extend_rect(rect, x[i], y[i]);
                    ADD_VERTEX(v, ci, x[i], y[i]);
                }

                // Generate indices
                sBatch.htriangle_fan(v0i, n - 2);
            }

            void Renderer::fill_rect(uint32_t ci, float x0, float y0, float x1, float y1)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0, y0);
                ADD_VERTEX(v, ci, x0, y1);
                ADD_VERTEX(v, ci, x1, y1);
                ADD_VERTEX(v, ci, x1, y0);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Renderer::fill_rect_textured(uint32_t ci, const texcoord_t & tex, float x0, float y0, float x1, float y1)
            {
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                const float tx0     = (x0 - tex.x) * tex.sx;
                const float tx1     = (x1 - tex.x) * tex.sx;
                const float ty0     = (y0 - tex.y) * tex.sy;
                const float ty1     = (y1 - tex.y) * tex.sy;

                ADD_TVERTEX(v, ci, x0, y0, tx0, ty0);
                ADD_TVERTEX(v, ci, x0, y1, tx0, ty1);
                ADD_TVERTEX(v, ci, x1, y1, tx1, ty1);
                ADD_TVERTEX(v, ci, x1, y0, tx1, ty0);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            void Renderer::fill_circle(uint32_t ci, float x, float y, float r)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float phi     = lsp_min(MATH_PI / r, MATH_PI_MUL_1D4);
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const size_t count  = MATH_PI_MUL_2 / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx = r;
                float vy = 0.0f;

                ADD_VERTEX(v, ci, x, y);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                for (size_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    ADD_VERTEX(v, ci, x + vx, y + vy);
                }

                ADD_VERTEX(v, ci, x + r, y);

                // Generate indices
                sBatch.htriangle_fan(v0i, count + 1);
            }

            void Renderer::fill_sector(uint32_t ci, float x, float y, float r, float a1, float a2)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float phi     = (delta > 0.0f) ? lsp_min(MATH_PI / r, MATH_PI_MUL_1D4) : lsp_min(-MATH_PI / r, MATH_PI_MUL_1D4);
                const float ex      = cosf(a2) * r;
                const float ey      = sinf(a2) * r;
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * r;
                float vy            = sinf(a1) * r;

                ADD_VERTEX(v, ci, x, y);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    ADD_VERTEX(v, ci, x + vx, y + vy);
                }

                ADD_VERTEX(v, ci, x + ex, y + ey);

                // Generate indices
                sBatch.htriangle_fan(v0i, count + 1);
            }

            void Renderer::fill_sector_textured(uint32_t ci, const texcoord_t & tex, float x, float y, float r, float a1, float a2)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float phi     = (delta > 0.0f) ? lsp_min(MATH_PI / r, MATH_PI_MUL_1D4) : lsp_min(-MATH_PI / r, MATH_PI_MUL_1D4);
                const float ex      = cosf(a2) * r;
                const float ey      = sinf(a2) * r;
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * r;
                float vy            = sinf(a1) * r;

                ADD_TVERTEX(v, ci, x, y, (x - tex.x) * tex.sx, (y - tex.y) * tex.sy);
                float xx            = x + vx;
                float yy            = y + vy;
                float txx           = (xx - tex.x) * tex.sx;
                float tyy           = (yy - tex.y) * tex.sy;
                ADD_TVERTEX(v, ci, xx, yy, txx, tyy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    xx          = x + vx;
                    yy          = y + vy;
                    txx         = (xx - tex.x) * tex.sx;
                    tyy         = (yy - tex.y) * tex.sy;
                    ADD_TVERTEX(v, ci, xx, yy, txx, tyy);
                }

                xx          = x + ex;
                yy          = y + ey;
                txx         = (xx - tex.x) * tex.sx;
                tyy         = (yy - tex.y) * tex.sy;
                ADD_TVERTEX(v, ci, xx, yy, txx, tyy);

                // Generate indices
                sBatch.htriangle_fan(v0i, count + 1);
            }

            void Renderer::fill_corner(uint32_t ci, float x, float y, float xd, float yd, float r, float a)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;

                const float delta   = MATH_PI_MUL_1D2;
                const float phi     = (delta > 0.0f) ? lsp_min(MATH_PI / r, MATH_PI_MUL_1D4) : lsp_min(-MATH_PI / r, MATH_PI_MUL_1D4);
                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count + 3);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a) * r;
                float vy            = sinf(a) * r;
                const float ex      = -vy;
                const float ey      = vx;

                ADD_VERTEX(v, ci, xd, yd);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    ADD_VERTEX(v, ci, x + vx, y + vy);
                }

                ADD_VERTEX(v, ci, x + ex, y + ey);

                // Generate indices
                sBatch.htriangle_fan(v0i, count + 1);
            }

            void Renderer::wire_line(uint32_t ci, float x0, float y0, float x1, float y1, float width)
            {
                // Find first not short segment
                width          *= 0.5f;
                const float dx  = x1 - x0;
                const float dy  = y1 - y0;
                const float d   = dx*dx + dy*dy;
                if (d <= 1e-10f)
                    return;

                // Draw first segment
                const float kd  = width / sqrtf(d);
                const float ndx = -dy * kd;
                const float ndy = dx * kd;

                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(4);
                if (v == NULL)
                    return;

                ADD_VERTEX(v, ci, x0 + ndx, y0 + ndy);
                ADD_VERTEX(v, ci, x0 - ndx, y0 - ndy);
                ADD_VERTEX(v, ci, x1 - ndx, y1 - ndy);
                ADD_VERTEX(v, ci, x1 + ndx, y1 + ndy);

                sBatch.hrectangle(vi, vi + 1, vi + 2, vi + 3);
            }

            template <class T>
            inline void Renderer::wire_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, const float *x, const float *y, float width, size_t n)
            {
                vertex_t *v     = vertices;
                T *iv           = indices;
                lsp_finally
                {
                    vertices        = v;
                    indices         = iv;
                };

                size_t i;
                float dx, dy, d;
                float kd, ndx, ndy;

                // Find first not short segment
                width          *= 0.5f;
                size_t si       = 0;
                for (i = 1; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                        break;
                }
                if (i >= n)
                    return;

                // Draw first segment
                kd              = width / sqrtf(d);
                ndx             = -dy * kd;
                ndy             = dx * kd;

                ADD_VERTEX(v, ci, x[i] + ndx, y[i] + ndy);
                ADD_VERTEX(v, ci, x[i] - ndx, y[i] - ndy);
                ADD_VERTEX(v, ci, x[si] - ndx, y[si] - ndy);
                ADD_VERTEX(v, ci, x[si] + ndx, y[si] + ndy);

                ADD_HRECTANGLE(iv, vi, vi + 1, vi + 2, vi + 3);
                si                  = i++;

                // Draw the rest segments
                for (; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                    {
                        kd              = width / sqrtf(d);
                        ndx             = -dy * kd;
                        ndy             = dx * kd;

                        ADD_VERTEX(v, ci, x[i] + ndx, y[i] + ndy);
                        ADD_VERTEX(v, ci, x[i] - ndx, y[i] - ndy);
                        ADD_VERTEX(v, ci, x[si] - ndx, y[si] - ndy);
                        ADD_VERTEX(v, ci, x[si] + ndx, y[si] + ndy);

                        ADD_HRECTANGLE(iv, vi + 4, vi + 5, vi + 6, vi + 7);
                        ADD_HRECTANGLE(iv, vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }
            }

            template <class T>
            inline void Renderer::wire_polyline(vertex_t * & vertices, T * & indices, T vi, uint32_t ci, clip_rect_t &rect, const float *x, const float *y, float width, size_t n)
            {
                vertex_t *v     = vertices;
                T *iv           = indices;
                lsp_finally
                {
                    vertices        = v;
                    indices         = iv;
                };

                size_t i;
                float dx, dy, d;
                float kd, ndx, ndy;
                float px, py;

                // Find first not short segment
                width          *= 0.5f;
                size_t si       = 0;
                for (i = 1; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                        break;
                }
                if (i >= n)
                    return;

                // Draw first segment
                kd              = width / sqrtf(d);
                ndx             = -dy * kd;
                ndy             = dx * kd;

                px              = x[i] + ndx;
                py              = y[i] + ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[i] - ndx;
                py              = y[i] - ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[si] - ndx;
                py              = y[si] - ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                px              = x[si] + ndx;
                py              = y[si] + ndy;
                extend_rect(rect, px, py);
                ADD_VERTEX(v, ci, px, py);

                ADD_HRECTANGLE(iv, vi, vi + 1, vi + 2, vi + 3);
                si                  = i++;

                // Draw the rest segments
                for (; i < n; ++i)
                {
                    dx              = x[i] - x[si];
                    dy              = y[i] - y[si];
                    d               = dx*dx + dy*dy;
                    if (d > 1e-10f)
                    {
                        kd              = width / sqrtf(d);
                        ndx             = -dy * kd;
                        ndy             = dx * kd;

                        px              = x[i] + ndx;
                        py              = y[i] + ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[i] - ndx;
                        py              = y[i] - ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[si] - ndx;
                        py              = y[si] - ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        px              = x[si] + ndx;
                        py              = y[si] + ndy;
                        extend_rect(rect, px, py);
                        ADD_VERTEX(v, ci, px, py);

                        ADD_HRECTANGLE(iv, vi + 4, vi + 5, vi + 6, vi + 7);
                        ADD_HRECTANGLE(iv, vi, vi + 6, vi + 1, vi + 7);

                        si              = i;
                        vi             += 4;
                    }
                }
            }

            void Renderer::wire_polyline(uint32_t ci, clip_rect_t & rect, const float *x, const float *y, float width, size_t n)
            {
                // Allocate vertices
                const uint32_t segs = n - 1;
                const uint32_t v_reserve = segs * 4;
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(v_reserve);
                if (v == NULL)
                    return;
                const vertex_t *v_tail = &v[v_reserve];
                lsp_finally {
                    if (v_tail > v)
                        sBatch.release_vertices(v_tail - v);
                };

                // Allocate indices
                const uint32_t iv_reserve = (2*segs - 1) * 6;
                void *iv_raw    = sBatch.add_indices(iv_reserve, vi + v_reserve - 1);
                if (iv_raw == NULL)
                    return;
                ssize_t iv_release = iv_reserve;
                lsp_finally {
                    if (iv_release > 0)
                        sBatch.release_indices(iv_release);
                };

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U8:
                    {
                        uint8_t *iv     = static_cast<uint8_t *>(iv_raw);
                        const uint8_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint8_t>(v, iv, uint8_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv    = static_cast<uint16_t *>(iv_raw);
                        const uint16_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint16_t>(v, iv, uint16_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv    = static_cast<uint32_t *>(iv_raw);
                        const uint32_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint32_t>(v, iv, uint32_t(vi), ci, rect, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    default:
                        break;
                }
            }

            void Renderer::wire_polyline(uint32_t ci, const float *x, const float *y, float width, size_t n)
            {
                // Allocate vertices
                const uint32_t segs = n - 1;
                const uint32_t v_reserve = segs * 4;
                const uint32_t vi   = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(v_reserve);
                if (v == NULL)
                    return;
                const vertex_t *v_tail = &v[v_reserve];
                lsp_finally {
                    if (v_tail > v)
                        sBatch.release_vertices(v_tail - v);
                };

                // Allocate indices
                const uint32_t iv_reserve = (2*segs - 1) * 6;
                void *iv_raw    = sBatch.add_indices(iv_reserve, vi + v_reserve - 1);
                if (iv_raw == NULL)
                    return;
                ssize_t iv_release = iv_reserve;
                lsp_finally {
                    if (iv_release > 0)
                        sBatch.release_indices(iv_release);
                };

                switch (sBatch.index_format())
                {
                    case INDEX_FMT_U8:
                    {
                        uint8_t *iv     = static_cast<uint8_t *>(iv_raw);
                        const uint8_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint8_t>(v, iv, uint8_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U16:
                    {
                        uint16_t *iv    = static_cast<uint16_t *>(iv_raw);
                        const uint16_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint16_t>(v, iv, uint16_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    case INDEX_FMT_U32:
                    {
                        uint32_t *iv    = static_cast<uint32_t *>(iv_raw);
                        const uint32_t *iv_tail = &iv[iv_reserve];
                        wire_polyline<uint32_t>(v, iv, uint32_t(vi), ci, x, y, width, n);
                        iv_release      = iv_tail - iv;
                        break;
                    }
                    default:
                        break;
                }
            }

            void Renderer::wire_arc(uint32_t ci, float x, float y, float r, float a1, float a2, float width)
            {
                // Compute parameters
                if (r <= 0.0f)
                    return;
                const float delta = a2 - a1;
                if (delta == 0.0f)
                    return;

                const float hw      = width * 0.5f;
                const float ro      = r + hw;
                const float kr      = lsp_max(r - hw, 0.0f) / ro;

                const float phi     = (delta > 0.0f) ? lsp_min(MATH_PI / ro, MATH_PI_MUL_1D4) : lsp_min(-MATH_PI / ro, MATH_PI_MUL_1D4);
                const float ex      = cosf(a2) * ro;
                const float ey      = sinf(a2) * ro;

                const float dx      = cosf(phi);
                const float dy      = sinf(phi);
                const ssize_t count = delta / phi;

                // Allocate resources
                const uint32_t v0i  = sBatch.next_vertex_index();
                vertex_t *v         = sBatch.add_vertices(count*2 + 4);
                if (v == NULL)
                    return;

                // Generate the geometry
                float vx            = cosf(a1) * ro;
                float vy            = sinf(a1) * ro;

                ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                ADD_VERTEX(v, ci, x + vx, y + vy);

                for (ssize_t i=0; i<count; ++i)
                {
                    float nvx   = vx*dx - vy*dy;
                    float nvy   = vx*dy + vy*dx;
                    vx          = nvx;
                    vy          = nvy;

                    ADD_VERTEX(v, ci, x + vx * kr, y + vy * kr);
                    ADD_VERTEX(v, ci, x + vx, y + vy);
                }

                ADD_VERTEX(v, ci, x + ex * kr, y + ey * kr);
                ADD_VERTEX(v, ci, x + ex, y + ey);

                // Generate indices
                sBatch.hrectangle_fan(v0i, count + 1);
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */
