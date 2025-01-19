/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-ws-lib
 * Created on: 19 янв. 2025 г.
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

#include <private/gl/glx_vtbl.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/stdlib/string.h>

#include <GL/glx.h>

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            vtbl_t *create_vtbl()
            {
                vtbl_t *vtbl = static_cast<vtbl_t *>(malloc(sizeof(vtbl_t)));
                if (vtbl == NULL)
                    return vtbl;
                bzero(vtbl, sizeof(vtbl_t));

                vtbl->glXGetProcAddress = reinterpret_cast<decltype(vtbl_t::glXGetProcAddress)>(
                        glXGetProcAddressARB(reinterpret_cast<const GLubyte *>("glXGetProcAddress")));

                #define FETCH(name) \
                    if (vtbl->glXGetProcAddress) \
                        vtbl->name  = reinterpret_cast<decltype(vtbl_t::name)>(vtbl->glXGetProcAddress(reinterpret_cast<const GLubyte *>(#name))); \
                    if (!vtbl->name) \
                        vtbl->name  = reinterpret_cast<decltype(vtbl_t::name)>(glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(#name)));

                FETCH(glXCreateContextAttribsARB);

                // Program operations
                FETCH(glCreateProgram);
                FETCH(glAttachShader);
                FETCH(glDetachShader);
                FETCH(glLinkProgram);
                FETCH(glUseProgram);
                FETCH(glGetProgramiv);
                FETCH(glGetProgramInfoLog);
                FETCH(glGetUniformLocation);
                FETCH(glDeleteProgram);

                // Shader operations
                FETCH(glCreateShader);
                FETCH(glShaderSource);
                FETCH(glCompileShader);
                FETCH(glGetShaderiv);
                FETCH(glGetShaderInfoLog);
                FETCH(glDeleteShader);

                FETCH(glUniform1f);
                FETCH(glUniform2f);
                FETCH(glUniform3f);
                FETCH(glUniform4f);
                FETCH(glUniform1i);
                FETCH(glUniform2i);
                FETCH(glUniform3i);
                FETCH(glUniform4i);
                FETCH(glUniform1ui);
                FETCH(glUniform2ui);
                FETCH(glUniform3ui);
                FETCH(glUniform4ui);
                FETCH(glUniform1fv);
                FETCH(glUniform2fv);
                FETCH(glUniform3fv);
                FETCH(glUniform4fv);
                FETCH(glUniform1iv);
                FETCH(glUniform2iv);
                FETCH(glUniform3iv);
                FETCH(glUniform4iv);
                FETCH(glUniform1uiv);
                FETCH(glUniform2uiv);
                FETCH(glUniform3uiv);
                FETCH(glUniform4uiv);
                FETCH(glUniformMatrix2fv);
                FETCH(glUniformMatrix2x3fv);
                FETCH(glUniformMatrix2x4fv);
                FETCH(glUniformMatrix3fv);
                FETCH(glUniformMatrix3x2fv);
                FETCH(glUniformMatrix3x4fv);
                FETCH(glUniformMatrix4fv);
                FETCH(glUniformMatrix4x2fv);
                FETCH(glUniformMatrix4x3fv);

                return vtbl;
            }

        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */


