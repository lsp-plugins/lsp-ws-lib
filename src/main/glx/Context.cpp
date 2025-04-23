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

#include <private/glx/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL_GLX

#include <private/glx/Context.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>

#include <private/glx/shaders.h>
#include <private/glx/vtbl.h>

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            static const int fb_rgba24x32[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 32, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgba24x24[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgba24x16[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgb16x24[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 6, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgb16x16[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 6, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgb15x24[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 5, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };
            static const int fb_rgb15x16[]   = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_X_RENDERABLE, True, GLX_RED_SIZE, 5, GLX_GREEN_SIZE, 5, GLX_BLUE_SIZE, 5, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None };

            static const int * const fb_params[] =
            {
                fb_rgba24x32,
                fb_rgba24x24,
                fb_rgba24x16,
                fb_rgb16x24,
                fb_rgb16x16,
                fb_rgb15x24,
                fb_rgb15x16,
                NULL
            };

            static const int glx_context_attribs[] =
            {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 1,
                None
            };

            typedef GLXContext (*glXCreateContextAttribsARBProc)(
                Display *dpy,
                GLXFBConfig config,
                GLXContext share_context,
                Bool direct,
                const int *attrib_list);

            static bool check_gl_extension(const char *list, const char *check)
            {
                const size_t len = strlen(check);

                while (true)
                {
                    const char *item = strstr(list, check);
                    if (item == NULL)
                        return false;
                    if ((item == list) || (item[-1] == ' '))
                    {
                        if ((item[len] == ' ') || (item[len] == '\0'))
                            return true;
                    }
                    list = &item[len];
                }
            }

            static GLXFBConfig choose_fb_config(Display *dpy, int screen)
            {
                GLXFBConfig result = NULL;
                int max_sample_buffers = -1;
                int max_samples = -1;
                const int max_multisampling = 8;

                for (const int * const *atts = fb_params; *atts != NULL; ++atts)
                {
                    // Get framebuffer configurations
                    int fbcount = 0;
                    GLXFBConfig* fb_list = glXChooseFBConfig(dpy, screen, *atts, &fbcount);
                    if ((fb_list == NULL) || (fbcount < 0))
                        continue;
                    lsp_finally { XFree(fb_list); };

                    // Scan for the best result
                    for (int i=0; i<fbcount; ++i)
                    {
                        GLXFBConfig fbc = fb_list[i];
                        int sample_buffers = 0;
                        int samples = 0;
                        ::glXGetFBConfigAttrib(dpy, fbc, GLX_SAMPLE_BUFFERS, &sample_buffers);
                        ::glXGetFBConfigAttrib(dpy, fbc, GLX_SAMPLES, &samples);
                        if (samples > max_multisampling)
                            continue;

                        if ((max_sample_buffers < 0) || ((sample_buffers >= max_sample_buffers) && (samples >= max_samples)))
                        {
                            result              = fbc;
                            max_sample_buffers  = sample_buffers;
                            max_samples         = samples;
                        }
                    }

                    // Check if we reached the desired result
                    if ((max_sample_buffers > 0) && (max_samples > 0))
                    {
                    #ifdef LSP_TRACE
                        int red_size = 0;
                        int green_size = 0;
                        int blue_size = 0;
                        int alpha_size = 0;
                        int depth_size = 0;
                        int stencil_size = 0;

                        ::glXGetFBConfigAttrib(dpy, result, GLX_RED_SIZE, &red_size);
                        ::glXGetFBConfigAttrib(dpy, result, GLX_GREEN_SIZE, &green_size);
                        ::glXGetFBConfigAttrib(dpy, result, GLX_BLUE_SIZE, &blue_size);
                        ::glXGetFBConfigAttrib(dpy, result, GLX_ALPHA_SIZE, &alpha_size);
                        ::glXGetFBConfigAttrib(dpy, result, GLX_DEPTH_SIZE, &depth_size);
                        ::glXGetFBConfigAttrib(dpy, result, GLX_STENCIL_SIZE, &stencil_size);

                        lsp_trace("Selected fb_config: rgba={%d, %d, %d, %d}, depth=%d, stencil=%d, multisampling={%d, %d}",
                            red_size, green_size, blue_size, alpha_size, depth_size, stencil_size, max_sample_buffers, max_samples);
                    #endif /* LSP_TRACE */

                        return result;
                    }
                }

                return NULL;
            }

            void Context::destroy(program_t *prg)
            {
                if (prg == NULL)
                    return;

                if (prg->nFlags & PF_PROGRAM)
                {
                    pVtbl->glDeleteProgram(prg->nProgramId);
                    lsp_gl_trace("glDeleteProgram(%d)", prg->nProgramId);
                }
                if (prg->nFlags & PF_VERTEX)
                {
                    pVtbl->glDeleteShader(prg->nVertexId);
                    lsp_gl_trace("glDeleteShader(%d)", prg->nVertexId);
                }
                if (prg->nFlags & PF_FRAGMENT)
                {
                    pVtbl->glDeleteShader(prg->nFragmentId);
                    lsp_gl_trace("glDeleteShader(%d)", prg->nFragmentId);
                }

                free(prg);
            }

            uint32_t Context::multisample() const
            {
                return nMultisample;
            }

            Context::Context(::Display *dpy, ::GLXContext ctx, ::Window window, vtbl_t *vtbl, uint32_t multisample)
                : IContext(vtbl)
            {
                pDisplay        = dpy;
                hContext        = ctx;
                hWindow         = window;
                nMultisample    = multisample;

                lsp_gl_trace("Created GLX context ptr=%p", this);
            }

            Context::~Context()
            {
                if (hContext != NULL)
                    lsp_error("Non-NULL context, need invalidate() call before destroying context");

                lsp_gl_trace("Destroyed GLX context ptr=%p", this);
            }

            void Context::cleanup()
            {
                IContext::cleanup();

                if (hContext == NULL)
                    return;

                // Destroy shaders and programs
                for (size_t i=0, n=vPrograms.size(); i<n; ++i)
                    destroy(vPrograms.uget(i));
                vPrograms.flush();

                // Destroy context
                ::glXMakeCurrent(pDisplay, None, NULL);
                ::glXDestroyContext(pDisplay, hContext);
                lsp_gl_trace("glXDestroyContext(%p)", hContext);
                hContext        = NULL;
                pDisplay        = NULL;
                hWindow         = None;
            }

            bool Context::active() const
            {
                if (!valid())
                    return false;

                GLXContext ctx = ::glXGetCurrentContext();
                return ctx == hContext;
            }

            status_t Context::activate()
            {
                if (hContext == NULL)
                    return STATUS_BAD_STATE;

                if (::glXGetCurrentContext() != hContext)
                {
                    if (!::glXMakeCurrent(pDisplay, hWindow, hContext))
                        return STATUS_UNKNOWN_ERR;
                }

                perform_gc();

                return STATUS_OK;
            }

            status_t Context::deactivate()
            {
                if (hContext == NULL)
                    return STATUS_OK;

                if (::glXGetCurrentContext() != hContext)
                    return STATUS_BAD_STATE;

                perform_gc();
                ::glXMakeCurrent(pDisplay, None, NULL);

                return STATUS_OK;
            }

            size_t Context::width() const
            {
                unsigned int width = 0;
                ::glXQueryDrawable(pDisplay, hWindow, GLX_WIDTH, &width);
                return width;
            }

            size_t Context::height() const
            {
                unsigned int height = 0;
                ::glXQueryDrawable(pDisplay, hWindow, GLX_HEIGHT, &height);
                return height;
            }

            void Context::swap_buffers(size_t width, size_t height)
            {
                pVtbl->glReadBuffer(GL_BACK);
                pVtbl->glDrawBuffer(GL_FRONT);
                pVtbl->glBlitFramebuffer(
                    0, 0, width, height,
                    0, 0, width, height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
                pVtbl->glFlush();

                // Enable this if you need to run something like RENDERDOC
                ::glXSwapBuffers(pDisplay, hWindow);
            }

            const char *Context::vertex_shader(size_t program_id)
            {
                switch (program_id)
                {
                    case gl::GEOMETRY: return glx::geometry_vertex_shader;
                    case gl::STENCIL: return glx::stencil_vertex_shader;
                    default: break;
                }
                return NULL;
            }

            const char *Context::fragment_shader(size_t program_id)
            {
                switch (program_id)
                {
                    case gl::GEOMETRY: return glx::geometry_fragment_shader;
                    case gl::STENCIL: return glx::stencil_fragment_shader;
                    default: break;
                }
                return NULL;
            }

            bool Context::check_gl_error(const char *context)
            {
                size_t count = 0;
                while (true)
                {
                    const GLenum error = pVtbl->glGetError();
                    if (error == GL_NO_ERROR)
                        return count > 0;

                    lsp_error("OpenGL error while performing operation '%s': code=0x%x", context, int(error));
                    ++count;
                }
            }

            bool Context::check_compile_status(const char *context, GLenum id, compile_status_t type)
            {
                int success;
                constexpr size_t MESSAGE_SIZE = 8192;

                if (type == SHADER)
                {
                    pVtbl->glGetShaderiv(id, GL_COMPILE_STATUS, &success);
                    if (success)
                        return false;

                    char *log = new char[MESSAGE_SIZE];
                    lsp_finally {
                        if (log != NULL)
                            delete[] log;
                    };

                    pVtbl->glGetShaderInfoLog(id, MESSAGE_SIZE, NULL, log);
                    lsp_error("OpenGL error while performing operation '%s':\n%s", context, log);
                    return true;
                }
                else if (type == PROGRAM)
                {
                    pVtbl->glGetProgramiv(id, GL_LINK_STATUS, &success);
                    if (success)
                        return false;

                    char *log = new char[MESSAGE_SIZE];
                    lsp_finally {
                        if (log != NULL)
                            delete[] log;
                    };

                    pVtbl->glGetProgramInfoLog(id, MESSAGE_SIZE, NULL, log);
                    lsp_error("OpenGL error while performing operation '%s':\n%s", context, log);
                    return true;
                }

                return false;
            }

            status_t Context::program(size_t *id, gl::program_t program)
            {
                if (!active())
                    return STATUS_BAD_STATE;

                const size_t index = size_t(program);
                program_t *prog = vPrograms.get(index);
                if (prog == NULL)
                {
                    // Obtain source code for shaders
                    const char *vertex  = vertex_shader(index);
                    if (vertex == NULL)
                    {
                        lsp_error("Vertex shader not defined for program id=%d", int(index));
                        return STATUS_BAD_STATE;
                    }

                    const char *fragment= fragment_shader(index);
                    if (fragment == NULL)
                    {
                        lsp_error("Fragment shader not defined for program id=%d", int(index));
                        return STATUS_BAD_STATE;
                    }

                    // Create new program
                    program_t *prg  = static_cast<program_t *>(malloc(sizeof(program_t)));
                    if (prg == NULL)
                        return STATUS_NO_MEM;

                    prg->nVertexId  = 0;
                    prg->nFragmentId= 0;
                    prg->nProgramId = 0;
                    prg->nFlags     = 0;
                    lsp_finally { destroy(prg); };

                    // Compile vertex shader
                    if ((prg->nVertexId = pVtbl->glCreateShader(GL_VERTEX_SHADER)) == None)
                    {
                        check_gl_error("create vertex shader");
                        return STATUS_UNKNOWN_ERR;
                    }
                    lsp_gl_trace("glCreateShader(%d)", int(prg->nVertexId));
                    prg->nFlags    |= PF_VERTEX;
                    pVtbl->glShaderSource(prg->nVertexId, 1, &vertex, NULL);
//                    if (check_gl_error("set vertex shader source"))
//                        return STATUS_UNKNOWN_ERR;
                    pVtbl->glCompileShader(prg->nVertexId);
                    if (check_compile_status("compile vertex shader", prg->nVertexId, SHADER))
                    {
                        lsp_trace("Vertex shader:\n%s", vertex);
                        check_gl_error("compile vertex shader");
                        return STATUS_UNKNOWN_ERR;
                    }
//                    if (check_gl_error("compile vertex shader"))
//                        return STATUS_UNKNOWN_ERR;

                    // Compile fragment shader
                    if ((prg->nFragmentId = pVtbl->glCreateShader(GL_FRAGMENT_SHADER)) == None)
                    {
                        check_gl_error("create fragment shader");
                        return STATUS_UNKNOWN_ERR;
                    }
                    lsp_gl_trace("glCreateShader(%d)", int(prg->nFragmentId));
                    prg->nFlags    |= PF_FRAGMENT;
                    pVtbl->glShaderSource(prg->nFragmentId, 1, &fragment, NULL);
//                    if (check_gl_error("set fragment shader source"))
//                        return STATUS_UNKNOWN_ERR;
                    pVtbl->glCompileShader(prg->nFragmentId);
                    if (check_compile_status("compile fragment shader", prg->nFragmentId, SHADER))
                    {
                        lsp_trace("Fragment shader:\n%s", fragment);
                        check_gl_error("compile fragment shader");
                        return STATUS_UNKNOWN_ERR;
                    }
//                    if (check_gl_error("compile fragment shader"))
//                        return STATUS_UNKNOWN_ERR;

                    // Link program
                    if ((prg->nProgramId = pVtbl->glCreateProgram()) == None)
                    {
                        check_gl_error("create program");
                        return STATUS_UNKNOWN_ERR;
                    }
                    lsp_gl_trace("glCreateProgram(%d)", int(prg->nFragmentId));
                    prg->nFlags    |= PF_PROGRAM;
                    pVtbl->glAttachShader(prg->nProgramId, prg->nVertexId);
//                    if (check_gl_error("attach vertex shader to program"))
//                        return STATUS_UNKNOWN_ERR;
                    pVtbl->glAttachShader(prg->nProgramId, prg->nFragmentId);
//                    if (check_gl_error("attach fragment shader to program"))
//                        return STATUS_UNKNOWN_ERR;
                    pVtbl->glLinkProgram(prg->nProgramId);
                    if (check_compile_status("link program", prg->nProgramId, PROGRAM))
                    {
                        lsp_trace("Vertex shader:\n%s", vertex);
                        lsp_trace("Fragment shader:\n%s", vertex);
                        check_gl_error("link program");
                        return STATUS_UNKNOWN_ERR;
                    }
//                    if (check_gl_error("link program"))
//                        return STATUS_UNKNOWN_ERR;

                    // Now we can delete compiled shaders
                    pVtbl->glDeleteShader(prg->nVertexId);
//                    if (check_gl_error("delete vertex shader"))
//                        return STATUS_UNKNOWN_ERR;
                    lsp_gl_trace("glDeleteShader(%d)", int(prg->nVertexId));
                    prg->nFlags    &= ~PF_VERTEX;

                    pVtbl->glDeleteShader(prg->nFragmentId);
//                    if (check_gl_error("delete fragment shader"))
//                        return STATUS_UNKNOWN_ERR;
                    lsp_gl_trace("glDeleteShader(%d)", int(prg->nFragmentId));
                    prg->nFlags    &= ~PF_FRAGMENT;

                    // Add program to list
                    const size_t count  = index + 1 - vPrograms.size();
                    if (count > 0)
                    {
                        program_t **ptr     = vPrograms.append_n(count);
                        if (ptr == NULL)
                            return STATUS_NO_MEM;
                        for (size_t i=0; i<count; ++i)
                            ptr[i]              = NULL;
                    }
                    if (!vPrograms.set(index, prg))
                        return STATUS_UNKNOWN_ERR;

                    prog            = release_ptr(prg);
                }

                *id = prog->nProgramId;

                return STATUS_OK;
            }

            gl::IContext *create_context(Display *dpy, int screen, Window window)
            {
                // Choose FBConfig
                GLXFBConfig fb_config = choose_fb_config(dpy, screen);
                if (fb_config == NULL)
                    return NULL;

                // Create virtual table
                glx::vtbl_t *vtbl       = glx::create_vtbl();
                if (vtbl == NULL)
                    return NULL;
                lsp_finally { free(vtbl); };

                // Try to create OpenGL 3.0+ context
                GLXContext ctx = NULL;
                const char *extensions = ::glXQueryExtensionsString(dpy, screen);
                if ((check_gl_extension(extensions, "GLX_ARB_create_context")) &&
                    (vtbl->glXCreateContextAttribsARB != NULL))
                {
                    ctx = vtbl->glXCreateContextAttribsARB(dpy, fb_config, 0, GL_TRUE, glx_context_attribs);
                    if (ctx == NULL)
                        ctx = vtbl->glXCreateContextAttribsARB(dpy, fb_config, 0, GL_FALSE, glx_context_attribs);
                }

                if (ctx == NULL)
                    ctx = ::glXCreateNewContext(dpy, fb_config, GLX_RGBA_TYPE, NULL, GL_TRUE);
                if (ctx == NULL)
                    ctx = ::glXCreateNewContext(dpy, fb_config, GLX_RGBA_TYPE, NULL, GL_FALSE);
                if (ctx == NULL)
                    return NULL;

                lsp_gl_trace("glXCreateContext(%p)", ctx);

                // Wrap the created context with context wrapper.
                int max_multisampling = 0;
                glXGetFBConfigAttrib(dpy, fb_config, GLX_SAMPLES, &max_multisampling);
                glx::Context *glx_ctx = new glx::Context(dpy, ctx, window, vtbl, max_multisampling);
                if (glx_ctx == NULL)
                {
                    ::glXDestroyContext(dpy, ctx);
                    return NULL;
                }

                vtbl            = NULL;
                return glx_ctx;
            }
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL_GLX */


