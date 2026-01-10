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
#include <private/gl/Texture.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class Renderer;

            class LSP_HIDDEN_MODIFIER SurfaceContext
            {
                private:
                    uatomic_t                           nReferences;        // Number of references
                    ipc::Condition                      sCondition;         // Drawing condition
                    lltl::ddeque<gl::actions::action_t> sActions;          // Drawing commands
                    gl::Renderer                       *pRenderer;          // Renderer
                    ws::IDrawable                      *pDrawable;          // Handle of drawable
                    gl::Texture                        *pTexture;           // Texture
                    gl::surface_size_t                  sSize;              // Surface size
                    gl::origin_t                        sOrigin;            // Drawing origin
                    gl::clip_state_t                    sClipping;          // Clipping state
                    mutable uatomic_t                   nIsRendering;       // Surface context is in the render queue
                    bool                                bIsDrawing;         // Context is currently in drawing state
                    bool                                bAntiAliasing;      // Anti-aliasing state
                    bool                                bNested;            // Nested flag

                public:
                    SurfaceContext(gl::Renderer * renderer, ws::IDrawable *drawable, size_t width, size_t height);
                    SurfaceContext(SurfaceContext * parent, size_t width, size_t height);
                    SurfaceContext(const SurfaceContext &) = delete;
                    SurfaceContext(SurfaceContext &&) = delete;
                    ~SurfaceContext();

                    SurfaceContext & operator = (const SurfaceContext &) = delete;
                    SurfaceContext & operator = (SurfaceContext &&) = delete;

                protected:
                    gl::actions::action_t  *push_action(gl::actions::action_type_t type);
                    void                    clear_actions();

                public:
                    uatomic_t   reference_up();
                    uatomic_t   reference_down();

                public:
                    /**
                     * Get drawable descriptor
                     */
                    ws::IDrawable              *drawable()              { return pDrawable;     }

                    /**
                     * Get drawing origin
                     * @return drawing origin
                     */
                    inline gl::origin_t &       origin()                { return sOrigin;       }

                    /**
                     * Get clipping state
                     * @return clipping state
                     */
                    inline gl::clip_state_t &   clipping()              { return sClipping;     }

                    /**
                     * Check that context is active
                     * @return true if context is active
                     */
                    inline bool                 is_drawing() const      { return bIsDrawing;    }

                    /**
                     * Check that surface context is in rendering state
                     * @return
                     */
                    inline bool                 is_rendering() const    { return atomic_load(&nIsRendering) != 0;   }

                    /**
                     * Check anti-aliasing is enabled
                     * @return true if anti-aliasing is enabled
                     */
                    inline bool                 antialiasing() const    { return bAntiAliasing; }

                    /**
                     * Set anti-aliasing
                     * @param enable enable/disable flag
                     */
                    inline void                 set_antialiasing(bool enable)
                    {
                        bAntiAliasing       = enable;
                    }

                    /**
                     * Get surface width
                     * @return surface width
                     */
                    inline size_t               width()                 { return sSize.width;   }

                    /**
                     * Get surface height
                     * @return surface height
                     */
                    inline size_t               height()                { return sSize.height;  }

                    /**
                     * Set surface size
                     * @param size surface size
                     */
                    void                        set_size(const gl::surface_size_t & size);

                    /**
                     * Get texture
                     * @return texture
                     */
                    inline      gl::Texture    *texture()               { return pTexture;      }

                    /**
                     * Set texture
                     * @param texture texture to set
                     */
                    void        set_texture(gl::Texture *texture);

                    /**
                     * Check that surface is valid
                     * @return true if surface is valid
                     */
                    inline bool valid() const                           { return (pDrawable != NULL) && (pDrawable->valid()); }

                    /**
                     * Check that context is nested
                     * @return true if context is nested
                     */
                    inline bool is_nested() const                       { return bNested; }

                public:
                    /**
                     * Invalidate surface state
                     */
                    void        invalidate();

                    /**
                     * Begin drawing on the context
                     * @return true if state has changed to drawing
                     */
                    bool        begin_draw();

                    /**
                     * End drawing on the context
                     */
                    void        end_draw();

                    /**
                     * Notify the surface context being put to render queue
                     */
                    void        begin_render();

                    /**
                     * Notify the surface context being removed from render queue
                     */
                    void        end_render();

                    /**
                     * Get number of commands in the queue
                     * @return number of commands in the queue
                     */
                    inline size_t action_count() const
                    {
                        return sActions.size();
                    }

                    /**
                     * Get current action
                     * @return current action
                     */
                    inline const gl::actions::action_t *current_action() const
                    {
                        return sActions.first();
                    }

                    /**
                     * Move to the next command
                     */
                    const gl::actions::action_t *next_action();

                    /**
                     * Add drawing command
                     * @tparam T command type
                     * @return pointer to newly allocated drawing command
                     */
                    template <typename T>
                    inline T *append_command()
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
