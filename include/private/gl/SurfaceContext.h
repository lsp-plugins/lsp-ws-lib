/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 3 янв. 2026 г.
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

#ifndef PRIVATE_GL_SURFACECONTEXT_H_
#define PRIVATE_GL_SURFACECONTEXT_H_

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ipc/Condition.h>

#include <lsp-plug.in/lltl/ddeque.h>
#include <private/gl/Actions.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class SurfaceContext
            {
                private:
                    uatomic_t                           nReferences;        // Number of references
                    lltl::ddeque<gl::actions::action_t> sCommands;          // Drawing commands
                    ipc::Condition                      sCondition;         // Drawing condition
                    bool                                bIsDrawing;         // Context is currently in drawing state

                public:
                    SurfaceContext();
                    SurfaceContext(const SurfaceContext &) = delete;
                    SurfaceContext(SurfaceContext &&) = delete;
                    ~SurfaceContext();

                    SurfaceContext & operator = (const SurfaceContext &) = delete;
                    SurfaceContext & operator = (SurfaceContext &&) = delete;

                protected:
                    gl::actions::action_t  *push_action(gl::actions::action_type_t type);

                public:
                    uatomic_t   reference_up();
                    uatomic_t   reference_down();

                public:
                    /**
                     * Check that context is active
                     * @return true if context is active
                     */
                    inline bool is_drawing() const          { return bIsDrawing;    }

                    /**
                     * Begin drawing on the context
                     */
                    void        begin_draw();

                    /**
                     * End drawing on the context
                     */
                    void        end_draw();

                    /**
                     * Clear all currently drawing commands
                     */
                    void        clear();

                    /**
                     * Get number of commands in the queue
                     * @return number of commands in the queue
                     */
                    inline size_t count() const
                    {
                        return sCommands.size();
                    }

                    /**
                     * Get current action
                     * @return current action
                     */
                    inline gl::actions::action_t *current()
                    {
                        return sCommands.first();
                    }

                    /**
                     * Move to the next command
                     */
                    void next();

                    /**
                     * Add drawing command
                     * @tparam T command type
                     * @return pointer to newly allocated drawing command
                     */
                    template <typename T>
                    inline T *add()
                    {
                        gl::actions::action_t *result = push_action(T::type_id);
                        return (result != NULL) ? reinterpret_cast<T *>(result->data) : NULL;
                    }
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */



#endif /* PRIVATE_GL_SURFACECONTEXT_H_ */
