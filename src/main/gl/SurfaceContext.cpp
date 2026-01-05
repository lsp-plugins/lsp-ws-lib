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
                bIsDrawing      = false;
            }

            SurfaceContext::~SurfaceContext()
            {
                clear();
            }

            gl::actions::action_t *SurfaceContext::push_action(gl::actions::action_type_t type)
            {
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

            void SurfaceContext::clear()
            {
                for (lltl::iterator<actions::action_t> it = sCommands.values(); it; ++it)
                    actions::destroy(it.get());
                sCommands.clear();
            }

            void SurfaceContext::next()
            {
                actions::destroy(sCommands.front());
                sCommands.pop_front();
            }

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */


