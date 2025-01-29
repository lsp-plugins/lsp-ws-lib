/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 16 янв. 2025 г.
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

#ifndef PRIVATE_GL_ICONTEXT_H_
#define PRIVATE_GL_ICONTEXT_H_

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            enum context_param_id_t
            {
                END = 0,
                DISPLAY = 1,
                SCREEN = 2,
                WINDOW = 3,
            };

            typedef struct context_param_t
            {
                context_param_id_t id;
                union
                {
                    void           *ptr;
                    const char     *text;
                    bool            flag;
                    signed int      sint;
                    unsigned int    uint;
                    signed long     slong;
                    unsigned long   ulong;
                };
            } context_param_t;


            enum shader_t
            {
                VERTEX_SHADER,
                FRAGMENT_SHADER,
            };

            class LSP_HIDDEN_MODIFIER IContext
            {
                private:
                    uatomic_t       nReferences;
                    bool            bActive;

                public:
                    IContext();
                    IContext(const IContext &) = delete;
                    IContext(IContext &&) = delete;
                    virtual ~IContext();

                    IContext & operator = (const IContext &) = delete;
                    IContext & operator = (IContext &&) = delete;

                public:
                    uatomic_t   reference_up();
                    uatomic_t   reference_down();

                protected:
                    virtual status_t    do_activate();
                    virtual status_t    do_deactivate();

                public:
                    /**
                     * Check if context is currently active
                     * @return true if context is currently active
                     */
                    inline bool active() const  { return bActive; }

                    /**
                     * Activate context
                     * @return status of operation
                     */
                    status_t activate();

                    /**
                     * Deactivate context
                     * @return status of operation
                     */
                    status_t deactivate();

                    /**
                     * Get shader program for rendering
                     * @param shader program
                     * @return pointer to shader program or NULL
                     */
                    virtual const char *shader(shader_t shader) const;
            };

            template <class T>
            T *safe_acquire(T *ptr)
            {
                if (ptr != NULL)
                    ptr->reference_up();
                return ptr;
            }

            template <class T>
            void safe_release(T * &ptr)
            {
                if (ptr == NULL)
                    return;

                ptr->reference_down();
                ptr = NULL;
            }

            /**
             * Create OpenGL context using specified params
             * @param params specified params
             * @return pointer to created context or NULL
             */
            gl::IContext *create_context(const context_param_t *params);

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* INCLUDE_PRIVATE_GL_ICONTEXT_H_ */
