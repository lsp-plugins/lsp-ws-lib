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
                SHADER("uniform mat4 u_viewport;")
                SHADER("")
                SHADER("in vec3 a_vertex;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    gl_Position = u_viewport * vec4(a_vertex.xy, 0.0f, 1.0f);")
                SHADER("}")
                SHADER("");

            static const char *geometry_fragment_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("uniform mat4 u_viewport;")
                SHADER("")
                SHADER("out vec4 FragColor;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    FragColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);")
                SHADER("}")
                SHADER("");

            #undef SHADER
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_GL_GLX_SHADERS_H_ */
