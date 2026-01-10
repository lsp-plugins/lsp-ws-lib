/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/ws/IDrawable.h>

#include <private/gl/Allocator.h>
#include <private/gl/Data.h>
#include <private/gl/Stats.h>
#include <private/gl/vtbl.h>

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            class Texture;

            class LSP_HIDDEN_MODIFIER IContext
            {
                protected:
                    typedef struct texture_t
                    {
                        GLuint          nId;
                        uint32_t        nSamples;
                    } texture_t;

                private:
                    uatomic_t           nReferences;
                    ipc::Mutex          sTexMutex;          // Texture mutex for allocation/deallocation

                    lltl::darray<GLuint> vFramebuffers;
                    lltl::darray<GLuint> vRenderbuffers;
                    lltl::darray<GLuint> vTextures;

                    lltl::darray<GLuint> vGcFramebuffers;
                    lltl::darray<GLuint> vGcRenderbuffers;
                    lltl::darray<GLuint> vGcTextures;

                    lltl::darray<texture_t> vEmpty;

                    GLuint              nCommandsId;        // Texture for loading commands
                    uint32_t            nCommandsSize;      // Size of the command texture
                    GLuint              nCommandsProcessor; // Commands processor

                protected:
                    const gl::vtbl_t   *pVtbl;

                protected:
                    static void         remove_identifiers(lltl::darray<GLuint> & ids, lltl::darray<GLuint> & list);

                public:
                    IContext(const gl::vtbl_t *vtbl);
                    IContext(const IContext &) = delete;
                    IContext(IContext &&) = delete;
                    virtual ~IContext();

                    IContext & operator = (const IContext &) = delete;
                    IContext & operator = (IContext &&) = delete;

                    virtual void destroy();

                public:
                    uatomic_t   reference_up();
                    uatomic_t   reference_down();

                private:
                    void        perform_gc();

                public:
                    /**
                     * Obtain virtual table of OpenGL functions
                     * @return virtual table of OpenGL functions
                     */
                    const vtbl_t *vtbl() const          { return pVtbl; };

                    /**
                     * Allocate framebuffer
                     * @return allocated framebuffer or 0 on error
                     */
                    GLuint alloc_framebuffer();

                    /**
                     * Allocate renderbuffer
                     * @return allocated renderbuffer or 0 on error
                     */
                    GLuint alloc_renderbuffer();

                    /**
                     * Allocate texture
                     * @return allocated texture or 0 on error
                     */
                    GLuint alloc_texture();

                    /**
                     * Put frame buffer to list of destruction
                     * @param id framebuffer identifier
                     */
                    void free_framebuffer(GLuint id);

                    /**
                     * Put render buffer to list of destruction
                     * @param id render buffer identifier
                     */
                    void free_renderbuffer(GLuint id);

                    /**
                     * Put texture to list of destruction
                     * @param id texture identifier
                     */
                    void free_texture(GLuint id);

                public:
                    /**
                     * Load commands to quadratic texture
                     * @param buf buffer of RGBA32F records
                     * @param size the size of one size of texture
                     * @param length the total number of floating-point values to load
                     * @return status of operation
                     */
                    status_t load_command_buffer(const float *buf, size_t size, size_t length);

                    /**
                     * Binds currently loaded command buffer to the specified texture processor
                     * @param processor_id texture processor identifier
                     * @return status of operation
                     */
                    status_t bind_command_buffer(GLuint processor_id);

                    /**
                     * Unbind command buffer
                     * @param processor_id texture processor identifier
                     */
                    void unbind_command_buffer();

                    /**
                     * Bind empty texture
                     * @param processor_id texture procesor identifier
                     * @param samples number of samples (multisampling factor)
                     * @return status of operation
                     */
                    status_t bind_empty_texture(GLuint processor_id, size_t samples);

                    /**
                     * Unbind empty texture
                     * @param processor_id texture processor identifier
                     * @param multisample multisampling flag
                     */
                    void unbind_empty_texture(GLuint processor_id, bool multisample);

                public:
                    /**
                     * Activate context
                     * @param drawable the drawable object to set up the context
                     * @return status of operation
                     */
                    virtual status_t activate(ws::IDrawable * drawable);

                    /**
                     * Deactivate context
                     * @param
                     */
                    virtual void deactivate();

                    /**
                     * Swap back and front buffer
                     * @param width width of the buffer
                     * @param height height of the buffer
                     */
                    virtual void swap_buffers(size_t width, size_t height);

                public:
                    /**
                     * Get shader program for rendering
                     * @param program shader program identifier
                     * @param id pointer to store the identifier of the program
                     * @return pointer identifier of shader program or negative error code
                     */
                    virtual status_t program(size_t *id, program_t program);

                    /**
                     * Get attribute location for specific program
                     * @param program shader program identifier
                     * @param attribute attribute identifier
                     * @return attribute location or negative error code
                     */
                    virtual GLint attribute_location(program_t program, attribute_t attribute);

                    /**
                     * Get multisampling factor
                     * @return multisampling factor
                     */
                    virtual uint32_t multisample() const;

                    /**
                     * Get the width of the associated drawable surface
                     * @return width of the associated drawable surface
                     */
                    virtual size_t width() const;

                    /**
                     * Get the hight of the associated drawable surface
                     * @return the hight of the associated drawable surface
                     */
                    virtual size_t height() const;
            };

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_ICONTEXT_H_ */
