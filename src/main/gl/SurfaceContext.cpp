/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 5 янв. 2026 г.
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

#include <private/gl/Renderer.h>
#include <private/gl/SurfaceContext.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
//            static uatomic_t nSurfaceContext = 0;

            SurfaceContext::SurfaceContext(gl::Renderer *renderer, ws::IDrawable *drawable, size_t width, size_t height)
            {
//                lsp_trace("this=%p, count=%d", this, int(atomic_add(&nSurfaceContext, 1) + 1));

                atomic_store(&nReferences, 1);
                pRenderer       = safe_acquire(renderer);
                pDrawable       = safe_acquire(drawable);
                pTexture        = NULL;

                bzero(sClipping.clips, sizeof(gl::clip_rect_t) * gl::clip_state_t::MAX_CLIPS);
                sSize.width     = width;
                sSize.height    = height;
                sOrigin.left    = 0;
                sOrigin.top     = 0;
                sClipping.count = 0;

                atomic_store(&nIsRendering, 0);
                bIsDrawing      = false;
                bAntiAliasing   = true;
                bNested         = false;
            }

            SurfaceContext::SurfaceContext(SurfaceContext * parent, size_t width, size_t height)
            {
//                lsp_trace("this=%p, count=%d", this, int(atomic_add(&nSurfaceContext, 1) + 1));

                atomic_store(&nReferences, 1);
                pRenderer       = safe_acquire(parent->pRenderer);
                pDrawable       = safe_acquire(parent->pDrawable);
                pTexture        = NULL;

                bzero(sClipping.clips, sizeof(gl::clip_rect_t) * gl::clip_state_t::MAX_CLIPS);
                sSize.width     = width;
                sSize.height    = height;
                sOrigin.left    = 0;
                sOrigin.top     = 0;
                sClipping.count = 0;

                atomic_store(&nIsRendering, 0);
                bIsDrawing      = false;
                bAntiAliasing   = true;
                bNested         = true;
            }

            SurfaceContext::~SurfaceContext()
            {
//                lsp_trace("this=%p, count=%d", this, int(atomic_add(&nSurfaceContext, -1) - 1));

                clear_actions();

                safe_release(pTexture);
                safe_release(pDrawable);
                safe_release(pRenderer);
            }

            gl::actions::action_t *SurfaceContext::push_action(gl::actions::action_type_t type)
            {
                if (!bIsDrawing)
                    return NULL;

                return actions::init(sActions.push_back(), type);
            }

            uatomic_t SurfaceContext::reference_up()
            {
                return atomic_add(&nReferences, 1) + 1;
            }

            uatomic_t SurfaceContext::reference_down()
            {
                uatomic_t result = atomic_add(&nReferences, -1) - 1;
                if (result == 0)
                    delete this;
                return result;
            }

            void SurfaceContext::invalidate()
            {
                if ((!bNested) && (pDrawable != NULL))
                    pDrawable->invalidate();
            }

            bool SurfaceContext::begin_draw()
            {
                if (bIsDrawing)
                    return false;

                // Wait until surface context is being removed from the render queue
                // Only after that we can perform additional manipulations
                wait();

                // Clear actions
                bIsDrawing     = true;
                clear_actions();

                return true;
            }

            void SurfaceContext::wait()
            {
                sCondition.lock();
                lsp_finally { sCondition.unlock(); };
                while (atomic_load(&nIsRendering) != 0)
                    sCondition.wait();
            }

            void SurfaceContext::end_draw()
            {
                if (!bIsDrawing)
                    return;

                // Submit self to the render queue
                bIsDrawing      = false;
                status_t res    = pRenderer->queue_draw(this);
                if (res != STATUS_OK)
                    clear_actions();
            }

            void SurfaceContext::begin_render()
            {
                sCondition.lock();
                lsp_finally { sCondition.unlock(); };
                atomic_add(&nIsRendering, 1);
            }

            void SurfaceContext::end_render()
            {
                // Clear actions first
                clear_actions();

                // Now we are ready to reset rendering state and unlock surface context
                sCondition.lock();
                lsp_finally { sCondition.unlock(); };

                atomic_add(&nIsRendering, -1);
                sCondition.notify();
            }

            void SurfaceContext::clear_actions()
            {
                for (lltl::iterator<actions::action_t> it = sActions.values(); it; ++it)
                    actions::destroy(it.get());
                sActions.clear();
            }

            const gl::actions::action_t *SurfaceContext::next_action()
            {
                actions::destroy(sActions.front());
                sActions.pop_front();
                return sActions.first();
            }

            void SurfaceContext::set_size(const gl::surface_size_t & size)
            {
                if ((size.width == sSize.width) &&
                    (size.height == sSize.height))
                    return;

                sSize       = size;
            }

            void SurfaceContext::set_texture(gl::Texture *texture)
            {
                safe_release(pTexture);
                pTexture = texture;
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */


