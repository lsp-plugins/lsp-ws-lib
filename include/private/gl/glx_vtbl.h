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

#ifndef PRIVATE_GL_VTBL_H_
#define PRIVATE_GL_VTBL_H_

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            typedef void (*procaddress_t)();

            typedef struct vtbl_t
            {
                procaddress_t (* glXGetProcAddress)(const GLubyte * procName);
                GLXContext (* glXCreateContextAttribsARB)(
                    Display *dpy,
                    GLXFBConfig config,
                    GLXContext share_context,
                    Bool direct,
                    const int *attrib_list);

                // Program operations
                GLuint (* glCreateProgram)();
                void (* glAttachShader)(GLuint program, GLuint shader);
                void (* glDetachShader)(GLuint program, GLuint shader);
                void (* glLinkProgram)(GLuint program);
                void (* glUseProgram)(GLuint program);
                void (* glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
                void (* glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                GLint (* glGetUniformLocation)( GLuint program, const GLchar *name);
                void (* glDeleteProgram)(GLuint program);

                // Shader operations
                GLuint (* glCreateShader)(GLenum shaderType);
                void (* glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
                GLuint (* glCompileShader)(GLuint shader);
                void (* glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
                void (* glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                void (* glDeleteShader)(GLuint shader);

                // Uniform operations
                void (* glUniform1f)(GLint location, GLfloat v0);
                void (* glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
                void (* glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
                void (* glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
                void (* glUniform1i)(GLint location, GLint v0);
                void (* glUniform2i)(GLint location, GLint v0, GLint v1);
                void (* glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
                void (* glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
                void (* glUniform1ui)(GLint location, GLuint v0);
                void (* glUniform2ui)(GLint location, GLuint v0, GLuint v1);
                void (* glUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
                void (* glUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
                void (* glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
                void (* glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
                void (* glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
                void (* glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
                void (* glUniform1iv)(GLint location, GLsizei count, const GLint *value);
                void (* glUniform2iv)(GLint location, GLsizei count, const GLint *value);
                void (* glUniform3iv)(GLint location, GLsizei count, const GLint *value);
                void (* glUniform4iv)(GLint location, GLsizei count, const GLint *value);
                void (* glUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
                void (* glUniform2uiv)(GLint location, GLsizei count, const GLuint *value);
                void (* glUniform3uiv)(GLint location, GLsizei count, const GLuint *value);
                void (* glUniform4uiv)(GLint location, GLsizei count, const GLuint *value);
                void (* glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void (* glUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

            } vtbl_t;

            /**
             * Create virtual table for calling extended OpenGL functions
             * @return virtual table
             */
            vtbl_t *create_vtbl();

        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */



#endif /* PRIVATE_GL_VTBL_H_ */
