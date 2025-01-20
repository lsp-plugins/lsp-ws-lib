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

#ifndef PRIVATE_GL_GLX_SHADERS_H_
#define PRIVATE_GL_GLX_SHADERS_H_

namespace lsp
{
    namespace ws
    {
        namespace glx
        {
            #define SHADER(x) x "\n"

            static const char *geometry_vertex_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("uniform mat4 u_model;")
                SHADER("")
                SHADER("in vec2 a_vertex;")
                SHADER("in vec2 a_texcoord;")
                SHADER("in uint a_command;")
                SHADER("")
                SHADER("out vec2 b_texcoord;")
                SHADER("flat out int b_index;")
                SHADER("flat out int b_coloring;")
                SHADER("flat out int b_clips;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    b_texcoord = a_texcoord;")
                SHADER("    b_index = int(a_command >> 5);")
                SHADER("    b_coloring = int(a_command >> 3) & 0x3;")
                SHADER("    b_clips = int(a_command & 0x7u);")
                SHADER("    gl_Position = u_model * vec4(a_vertex, 0.0f, 1.0f);")
                SHADER("}")
                SHADER("");

            static const char *geometry_fragment_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("uniform samplerBuffer u_buf_commands;")
                SHADER("")
                SHADER("in vec2 b_texcoord;")
                SHADER("flat in int b_index;")
                SHADER("flat in int b_coloring;")
                SHADER("flat in int b_clips;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    if (b_coloring == 0)")
                SHADER("    {")
                SHADER("        gl_FragColor = texelFetch(u_buf_commands, b_index);")
                SHADER("    }")
                SHADER("    else")
                SHADER("    {")
                SHADER("        gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);")
                SHADER("    }")
                SHADER("}")
                SHADER("");

            #undef SHADER
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_GL_GLX_SHADERS_H_ */
