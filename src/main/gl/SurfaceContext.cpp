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

#include <private/gl/SurfaceContext.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            SurfaceContext::SurfaceContext()
            {
                atomic_store(&nReferences, 1);
                pTexture        = NULL;

                bzero(sClipping.clips, sizeof(gl::clip_rect_t) * gl::clip_state_t::MAX_CLIPS);
                sSize.width     = 0;
                sSize.height    = 0;
                sOrigin.left    = 0;
                sOrigin.top     = 0;
                sClipping.count = 0;

                bzero(&sMatrix, sizeof(sMatrix));

                bIsDrawing      = false;
                bAntiAliasing   = true;
            }

            SurfaceContext::~SurfaceContext()
            {
                safe_release(pTexture);
                clear();
            }

            gl::actions::action_t *SurfaceContext::push_action(gl::actions::action_type_t type)
            {
                if (!bIsDrawing)
                    return NULL;

                return actions::init(sCommands.push_back(), type);
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

            void SurfaceContext::begin_draw()
            {
                bIsDrawing     = true;
            }

            void SurfaceContext::end_draw()
            {
                bIsDrawing     = false;
            }

            void SurfaceContext::begin_render(gl::IContext * context)
            {
                // TODO: activate context for a window
            }

            void SurfaceContext::end_render()
            {
            }

            void SurfaceContext::clear()
            {
                for (lltl::iterator<actions::action_t> it = sCommands.values(); it; ++it)
                    actions::destroy(it.get());
                sCommands.clear();
            }

            const gl::actions::action_t *SurfaceContext::next()
            {
                actions::destroy(sCommands.front());
                sCommands.pop_front();
                return sCommands.first();
            }

            bool SurfaceContext::fetch(gl::actions::action_t & action)
            {
                return sCommands.pop_front(action);
            }

            void SurfaceContext::set_size(const gl::surface_size_t & size)
            {
                if ((size.width == sSize.width) &&
                    (size.height == sSize.height))
                    return;

                sSize       = size;
                float *m    = sMatrix.v;

                // Set-up drawing matrix
                const float dx = 2.0f / float(size.width);
                const float dy = 2.0f / float(size.height);

                m[0]        = dx;
                m[1]        = 0.0f;
                m[2]        = 0.0f;
                m[3]        = 0.0f;

                m[4]        = 0.0f;
                m[5]        = -dy;
                m[6]        = 0.0f;
                m[7]        = 0.0f;

                m[8]        = 0.0f;
                m[9]        = 0.0f;
                m[10]       = 1.0f;
                m[11]       = 0.0f;

                m[12]       = -1.0f;
                m[13]       = 1.0f;
                m[14]       = 0.0f;
                m[15]       = 1.0f;
            }

            void SurfaceContext::set_texture(gl::Texture *texture)
            {
                safe_release(pTexture);
                pTexture = safe_acquire(texture);
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */


