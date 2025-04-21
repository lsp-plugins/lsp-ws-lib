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

#include <private/gl/defs.h>

#ifdef LSP_PLUGINS_USE_OPENGL

namespace lsp
{
    namespace ws
    {
        namespace gl
        {
            typedef struct vtbl_t
            {
                // Program operations
                GLuint GLAPIENTRY       (* glCreateProgram)();
                void GLAPIENTRY         (* glAttachShader)(GLuint program, GLuint shader);
                void GLAPIENTRY         (* glDetachShader)(GLuint program, GLuint shader);
                void GLAPIENTRY         (* glLinkProgram)(GLuint program);
                void GLAPIENTRY         (* glUseProgram)(GLuint program);
                void GLAPIENTRY         (* glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
                void GLAPIENTRY         (* glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                GLint GLAPIENTRY        (* glGetAttribLocation)(GLuint program, const GLchar *name);
                GLint GLAPIENTRY        (* glGetUniformLocation)(GLuint program, const GLchar *name);
                void GLAPIENTRY         (* glDeleteProgram)(GLuint program);

                // Shader operations
                GLuint GLAPIENTRY       (* glCreateShader)(GLenum shaderType);
                void GLAPIENTRY         (* glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
                GLuint GLAPIENTRY       (* glCompileShader)(GLuint shader);
                void GLAPIENTRY         (* glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
                void GLAPIENTRY         (* glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
                void GLAPIENTRY         (* glDeleteShader)(GLuint shader);

                // Uniform operations
                void GLAPIENTRY         (* glUniform1f)(GLint location, GLfloat v0);
                void GLAPIENTRY         (* glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
                void GLAPIENTRY         (* glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
                void GLAPIENTRY         (* glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
                void GLAPIENTRY         (* glUniform1i)(GLint location, GLint v0);
                void GLAPIENTRY         (* glUniform2i)(GLint location, GLint v0, GLint v1);
                void GLAPIENTRY         (* glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
                void GLAPIENTRY         (* glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
                void GLAPIENTRY         (* glUniform1ui)(GLint location, GLuint v0);
                void GLAPIENTRY         (* glUniform2ui)(GLint location, GLuint v0, GLuint v1);
                void GLAPIENTRY         (* glUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
                void GLAPIENTRY         (* glUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
                void GLAPIENTRY         (* glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
                void GLAPIENTRY         (* glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
                void GLAPIENTRY         (* glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
                void GLAPIENTRY         (* glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
                void GLAPIENTRY         (* glUniform1iv)(GLint location, GLsizei count, const GLint *value);
                void GLAPIENTRY         (* glUniform2iv)(GLint location, GLsizei count, const GLint *value);
                void GLAPIENTRY         (* glUniform3iv)(GLint location, GLsizei count, const GLint *value);
                void GLAPIENTRY         (* glUniform4iv)(GLint location, GLsizei count, const GLint *value);
                void GLAPIENTRY         (* glUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
                void GLAPIENTRY         (* glUniform2uiv)(GLint location, GLsizei count, const GLuint *value);
                void GLAPIENTRY         (* glUniform3uiv)(GLint location, GLsizei count, const GLuint *value);
                void GLAPIENTRY         (* glUniform4uiv)(GLint location, GLsizei count, const GLuint *value);
                void GLAPIENTRY         (* glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
                void GLAPIENTRY         (* glUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

                // Framebuffer operations
                void GLAPIENTRY         (* glGenFramebuffers)(GLsizei n, GLuint *ids);
                void GLAPIENTRY         (* glBindFramebuffer)(GLenum target, GLuint framebuffer);
                void GLAPIENTRY         (* glDeleteFramebuffers)(GLsizei n, GLuint *framebuffers);
                void GLAPIENTRY         (* glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
                void GLAPIENTRY         (* glFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
                void GLAPIENTRY         (* glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
                void GLAPIENTRY         (* glFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
                void GLAPIENTRY         (* glNamedFramebufferTexture)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
                void GLAPIENTRY         (* glDrawBuffers)(GLsizei n, const GLenum *bufs);
                void GLAPIENTRY         (* glNamedFramebufferDrawBuffers)(GLuint framebuffer, GLsizei n, const GLenum *bufs);
                GLenum GLAPIENTRY       (* glCheckFramebufferStatus)(GLenum target);
                GLenum GLAPIENTRY       (* glCheckNamedFramebufferStatus)(GLuint framebuffer, GLenum target);

                // Renderbuffer operations
                void GLAPIENTRY         (* glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
                void GLAPIENTRY         (* glDeleteRenderbuffers)(GLsizei n, GLuint *renderbuffers);
                void GLAPIENTRY         (* glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
                void GLAPIENTRY         (* glNamedFramebufferRenderbuffer)(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
                void GLAPIENTRY         (* glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
                void GLAPIENTRY         (* glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
                void GLAPIENTRY         (* glNamedRenderbufferStorage)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
                void GLAPIENTRY         (* glRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
                void GLAPIENTRY         (* glNamedRenderbufferStorageMultisample)(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

                // Buffer operations
                void GLAPIENTRY         (* glGenBuffers)(GLsizei n, GLuint * buffers);
                void GLAPIENTRY         (* glBindBuffer)(GLenum target, GLuint buffer);
                void GLAPIENTRY         (* glBufferData)(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
                void GLAPIENTRY         (* glNamedBufferData)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
                void GLAPIENTRY         (* glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
                void GLAPIENTRY         (* glNamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
                void *GLAPIENTRY        (* glMapBuffer)(GLenum target, GLenum access);
                void *GLAPIENTRY        (* glMapNamedBuffer)(GLuint buffer, GLenum access);
                void *GLAPIENTRY        (* glMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
                void *GLAPIENTRY        (* glMapNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
                void GLAPIENTRY         (* glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
                void GLAPIENTRY         (* glFlushMappedNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length);
                GLboolean GLAPIENTRY    (* glUnmapBuffer)(GLenum target);
                GLboolean GLAPIENTRY    (* glUnmapNamedBuffer)(GLuint buffer);
                void GLAPIENTRY         (* glDeleteBuffers)(GLsizei n, const GLuint * buffers);
                void GLAPIENTRY         (* glDrawBuffer)(GLenum buf);
                void GLAPIENTRY         (* glNamedFramebufferDrawBuffer)(GLuint framebuffer, GLenum buf);
                void GLAPIENTRY         (* glReadBuffer)(GLenum mode);
                void GLAPIENTRY         (* glNamedFramebufferReadBuffer)(GLuint framebuffer, GLenum mode);

                // Texture operations
                void GLAPIENTRY         (* glGenTextures)(GLsizei n, GLuint * textures);
                void GLAPIENTRY         (* glActiveTexture)(GLenum texture);
                void GLAPIENTRY         (* glTexBuffer)(GLenum target, GLenum internalformat, GLuint buffer);
                void GLAPIENTRY         (* glTextureBuffer)(GLuint texture, GLenum internalformat, GLuint buffer);
                void GLAPIENTRY         (* glTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void * data);
                void GLAPIENTRY         (* glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data);
                void GLAPIENTRY         (* glTexImage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
                void GLAPIENTRY         (* glTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * data);
                void GLAPIENTRY         (* glTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void * pixels);
                void GLAPIENTRY         (* glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels);
                void GLAPIENTRY         (* glTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels);
                void GLAPIENTRY         (* glTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
                void GLAPIENTRY         (* glTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
                void GLAPIENTRY         (* glTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
                void GLAPIENTRY         (* glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
                void GLAPIENTRY         (* glTexParameteri)(GLenum target, GLenum pname, GLint param);
                void GLAPIENTRY         (* glTextureParameterf)(GLuint texture, GLenum pname, GLfloat param);
                void GLAPIENTRY         (* glTextureParameteri)(GLuint texture, GLenum pname, GLint param);
                void GLAPIENTRY         (* glTexParameterfv)(GLenum target, GLenum pname, const GLfloat * params);
                void GLAPIENTRY         (* glTexParameteriv)(GLenum target, GLenum pname, const GLint * params);
                void GLAPIENTRY         (* glTexParameterIiv)(GLenum target, GLenum pname, const GLint * params);
                void GLAPIENTRY         (* glTexParameterIuiv)(GLenum target, GLenum pname, const GLuint * params);
                void GLAPIENTRY         (* glTextureParameterfv)(GLuint texture, GLenum pname, const GLfloat *params);
                void GLAPIENTRY         (* glTextureParameteriv)(GLuint texture, GLenum pname, const GLint *params);
                void GLAPIENTRY         (* glTextureParameterIiv)(GLuint texture, GLenum pname, const GLint *params);
                void GLAPIENTRY         (* glTextureParameterIuiv)(GLuint texture, GLenum pname, const GLuint *params);
                void GLAPIENTRY         (* glBindTexture)(GLenum target, GLuint texture);
                void GLAPIENTRY         (* glDeleteTextures)(GLsizei n, const GLuint * textures);

                // Vertex array operations
                void GLAPIENTRY         (* glGenVertexArrays)(GLsizei n, GLuint *arrays);
                void GLAPIENTRY         (* glBindVertexArray)(GLuint array);
                void GLAPIENTRY         (* glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
                void GLAPIENTRY         (* glEnableVertexAttribArray)(GLuint index);
                void GLAPIENTRY         (* glEnableVertexArrayAttrib)(GLuint vaobj, GLuint index);
                void GLAPIENTRY         (* glDisableVertexAttribArray)(GLuint index);
                void GLAPIENTRY         (* glDisableVertexArrayAttrib)(GLuint vaobj, GLuint index);
                void GLAPIENTRY         (* glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
                void GLAPIENTRY         (* glVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer);
                void GLAPIENTRY         (* glVertexAttribLPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer);

                // Miscellaneous functions
                void GLAPIENTRY         (* glPixelStoref)(GLenum pname, GLfloat param);
                void GLAPIENTRY         (* glPixelStorei)(GLenum pname, GLint param);
                void GLAPIENTRY         (* glGetBooleanv)(GLenum pname, GLboolean * data);
                void GLAPIENTRY         (* glGetDoublev)(GLenum pname, GLdouble * data);
                void GLAPIENTRY         (* glGetFloatv)(GLenum pname, GLfloat * data);
                void GLAPIENTRY         (* glGetIntegerv)(GLenum pname, GLint * data);
                void GLAPIENTRY         (* glGetInteger64v)(GLenum pname, GLint64 * data);
                void GLAPIENTRY         (* glGetBooleani_v)(GLenum target, GLuint index, GLboolean * data);
                void GLAPIENTRY         (* glGetIntegeri_v)(GLenum target, GLuint index, GLint * data);
                void GLAPIENTRY         (* glGetFloati_v)(GLenum target, GLuint index, GLfloat * data);
                void GLAPIENTRY         (* glGetDoublei_v)(GLenum target, GLuint index, GLdouble * data);
                void GLAPIENTRY         (* glGetInteger64i_v)(GLenum target, GLuint index, GLint64 * data);
                GLenum GLAPIENTRY       (* glGetError)();

                // Drawing operations
                const GLubyte           (* glGetString)(GLenum name);
                const GLubyte           (* glGetStringi)(GLenum name, GLuint index);
                void GLAPIENTRY         (* glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
                void GLAPIENTRY         (* glClear)(GLbitfield mask);
                void GLAPIENTRY         (* glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
                void GLAPIENTRY         (* glBlitNamedFramebuffer)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
                void GLAPIENTRY         (* glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
                void GLAPIENTRY         (* glFlush)();
                void GLAPIENTRY         (* glFinish)();
                void GLAPIENTRY         (* glEnable)(GLenum cap);
                void GLAPIENTRY         (* glDisable)(GLenum cap);
                void GLAPIENTRY         (* glEnablei)(GLenum cap, GLuint index);
                void GLAPIENTRY         (* glDisablei)(GLenum cap, GLuint index);
                void GLAPIENTRY         (* glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void * indices);
                void GLAPIENTRY         (* glStencilMask)(GLuint mask);
                void GLAPIENTRY         (* glBlendFunc)(GLenum sfactor, GLenum dfactor);
                void GLAPIENTRY         (* glBlendFunci)(GLuint buf, GLenum sfactor, GLenum dfactor);
                void GLAPIENTRY         (* glStencilOp)(GLenum sfail, GLenum dpfail, GLenum dppass);
                void GLAPIENTRY         (* glStencilFunc)(GLenum func, GLint ref, GLuint mask);
                void GLAPIENTRY         (* glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
                void GLAPIENTRY         (* glColorMaski)(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
            } vtbl_t;

        } /* namespace gl */
    } /* namespace ws */
} /* namespace lsp */

#endif /* LSP_PLUGINS_USE_OPENGL */

#endif /* PRIVATE_GL_VTBL_H_ */
