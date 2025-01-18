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

#include <private/gl/GLXContext.h>

#if defined(USE_LIBX11)

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>

#include <GL/glx.h>

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
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                None
            };

            static const int glx_legacy_context_attribs[] =
            {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
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
                        glXGetFBConfigAttrib(dpy, fbc, GLX_SAMPLE_BUFFERS, &sample_buffers);
                        glXGetFBConfigAttrib(dpy, fbc, GLX_SAMPLES, &samples);

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

                        glXGetFBConfigAttrib(dpy, result, GLX_RED_SIZE, &red_size);
                        glXGetFBConfigAttrib(dpy, result, GLX_GREEN_SIZE, &green_size);
                        glXGetFBConfigAttrib(dpy, result, GLX_BLUE_SIZE, &blue_size);
                        glXGetFBConfigAttrib(dpy, result, GLX_ALPHA_SIZE, &alpha_size);
                        glXGetFBConfigAttrib(dpy, result, GLX_DEPTH_SIZE, &depth_size);
                        glXGetFBConfigAttrib(dpy, result, GLX_STENCIL_SIZE, &stencil_size);

                        lsp_trace("Selected fb_config: rgba={%d, %d, %d, %d}, depth=%d, stencil=%d, multisampling={%d, %d}",
                            red_size, green_size, blue_size, alpha_size, depth_size, stencil_size, max_sample_buffers, max_samples);
                    #endif /* LSP_TRACE */

                        return result;
                    }
                }

                return NULL;
            }


            Context::Context(::Display *dpy, ::GLXContext ctx, ::Window window)
                : IContext()
            {
                pDisplay        = dpy;
                hContext        = ctx;
                hWindow         = window;
            }

            Context::~Context()
            {
                if (hContext != NULL)
                {
                    glXDestroyContext(pDisplay, hContext);
                    hContext        = NULL;
                }
                pDisplay        = NULL;
                hWindow         = None;
            }

            status_t Context::do_activate()
            {
                if (!::glXMakeCurrent(pDisplay, hWindow, hContext))
                    return STATUS_UNKNOWN_ERR;
                ::glDrawBuffer(GL_BACK);

                return STATUS_OK;
            }

            status_t Context::do_deactivate()
            {
                ::glXSwapBuffers(pDisplay, hWindow);
                ::glXMakeCurrent(pDisplay, None, NULL);

                return STATUS_NOT_IMPLEMENTED;
            }

            const char *Context::shader(gl::shader_t shader) const
            {
                return NULL;
            }

            gl::IContext *create_context(Display *dpy, int screen, Window window)
            {
                // Choose FBConfig
                GLXFBConfig fb_config = choose_fb_config(dpy, screen);
                if (fb_config == NULL)
                    return NULL;

                // Try to create OpenGL 3.0+ context
                GLXContext ctx = NULL;

                const char *extensions = glXQueryExtensionsString(dpy, screen);
                if (check_gl_extension(extensions, "GLX_ARB_create_context"))
                {
                    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
                    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
                             glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

                    if (glXCreateContextAttribsARB)
                    {
                        ctx = glXCreateContextAttribsARB(dpy, fb_config, 0, GL_TRUE, glx_context_attribs );
                        if (ctx == NULL)
                            ctx = glXCreateContextAttribsARB(dpy, fb_config, 0, GL_FALSE, glx_context_attribs );
                        if (ctx == NULL)
                            ctx = glXCreateContextAttribsARB(dpy, fb_config, 0, GL_TRUE, glx_legacy_context_attribs );
                        if (ctx == NULL)
                            ctx = glXCreateContextAttribsARB(dpy, fb_config, 0, GL_FALSE, glx_legacy_context_attribs );
                    }
                }

                if (ctx == NULL)
                    ctx = ::glXCreateNewContext(dpy, fb_config, GLX_RGBA_TYPE, NULL, GL_TRUE);
                if (ctx == NULL)
                    ctx = ::glXCreateNewContext(dpy, fb_config, GLX_RGBA_TYPE, NULL, GL_FALSE);
                if (ctx == NULL)
                    return NULL;

                // Wrap the created context with context wrapper.
                glx::Context *glx_ctx = new glx::Context(dpy, ctx, window);
                if (glx_ctx == NULL)
                {
                    glXDestroyContext(dpy, ctx);
                    return NULL;
                }

                return glx_ctx;
            }
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* USE_LIBX11 */


