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

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            typedef struct vtbl_t
            {
                // Program operations
                GLuint      (* glCreateProgram)();
                void        (* glAttachShader)(GLuint program, GLuint shader);
                void        (* glDetachShader)(GLuint program, GLuint shader);
                void        (* glLinkProgram)(GLuint program);
                void        (* glUseProgram)(GLuint program);
                void        (* glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
                void        (* glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                GLint       (* glGetAttribLocation)(GLuint program, const GLchar *name);
                GLint       (* glGetUniformLocation)(GLuint program, const GLchar *name);
                void        (* glDeleteProgram)(GLuint program);

                // Shader operations
                GLuint      (* glCreateShader)(GLenum shaderType);
                void        (* glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
                GLuint      (* glCompileShader)(GLuint shader);
                void        (* glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
                void        (* glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                void        (* glDeleteShader)(GLuint shader);

                // Uniform operations
                void        (* glUniform1f)(GLint location, GLfloat v0);
                void        (* glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
                void        (* glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
                void        (* glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
                void        (* glUniform1i)(GLint location, GLint v0);
                void        (* glUniform2i)(GLint location, GLint v0, GLint v1);
                void        (* glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
                void        (* glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
                void        (* glUniform1ui)(GLint location, GLuint v0);
                void        (* glUniform2ui)(GLint location, GLuint v0, GLuint v1);
                void        (* glUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
                void        (* glUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
                void        (* glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
                void        (* glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
                void        (* glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
                void        (* glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
                void        (* glUniform1iv)(GLint location, GLsizei count, const GLint *value);
                void        (* glUniform2iv)(GLint location, GLsizei count, const GLint *value);
                void        (* glUniform3iv)(GLint location, GLsizei count, const GLint *value);
                void        (* glUniform4iv)(GLint location, GLsizei count, const GLint *value);
                void        (* glUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
                void        (* glUniform2uiv)(GLint location, GLsizei count, const GLuint *value);
                void        (* glUniform3uiv)(GLint location, GLsizei count, const GLuint *value);
                void        (* glUniform4uiv)(GLint location, GLsizei count, const GLuint *value);
                void        (* glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void        (* glUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

                // Buffer operations
                void        (* glGenBuffers)(GLsizei n, GLuint * buffers);
                void        (* glBindBuffer)(GLenum target, GLuint buffer);
                void        (* glBufferData)(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
                void        (* glNamedBufferData)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
                void        (* glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
                void        (* glNamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
                void       *(* glMapBuffer)(GLenum target, GLenum access);
                void       *(* glMapNamedBuffer)(GLuint buffer, GLenum access);
                void       *(* glMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
                void       *(* glMapNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
                void        (* glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
                void        (* glFlushMappedNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length);
                GLboolean   (* glUnmapBuffer)(GLenum target);
                GLboolean   (* glUnmapNamedBuffer)(GLuint buffer);
                void        (* glDeleteBuffers)(GLsizei n, const GLuint * buffers);

                // Vertex array operations
                void        (* glGenVertexArrays)(GLsizei n, GLuint *arrays);
                void        (* glBindVertexArray)(GLuint array);
                void        (* glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
                void        (* glEnableVertexAttribArray)(GLuint index);
                void        (* glEnableVertexArrayAttrib)(GLuint vaobj, GLuint index);
                void        (* glDisableVertexAttribArray)(GLuint index);
                void        (* glDisableVertexArrayAttrib)(GLuint vaobj, GLuint index);
                void        (* glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
                void        (* glVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer);
                void        (* glVertexAttribLPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer);

            } vtbl_t;

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */



#endif /* PRIVATE_GL_VTBL_H_ */
