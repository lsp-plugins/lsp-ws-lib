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
                SHADER("out vec2 b_frag_coord;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    b_texcoord = a_texcoord;")
                SHADER("    b_index = int(a_command >> 5);")
                SHADER("    b_coloring = int(a_command >> 3) & 0x3;")
                SHADER("    b_clips = int(a_command & 0x7u);")
                SHADER("    b_frag_coord = a_vertex;")
                SHADER("")
                SHADER("    gl_Position = u_model * vec4(a_vertex, 0.0f, 1.0f);")
                SHADER("}")
                SHADER("");

            static const char *geometry_fragment_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("uniform samplerBuffer u_buf_commands;")
                SHADER("uniform sampler2D u_texture;")
                SHADER("uniform sampler2DMS u_ms_texture;")
                SHADER("")
                SHADER("in vec2 b_texcoord;")
                SHADER("flat in int b_index;")
                SHADER("flat in int b_coloring;")
                SHADER("flat in int b_clips;")
                SHADER("in vec2 b_frag_coord;")
                SHADER("")
                SHADER("vec4 textureMultisample(sampler2DMS sampler, vec2 coord, float factor)")
                SHADER("{")
                SHADER("    vec4 color = vec4(0.0);")
                SHADER("    ivec2 tsize = textureSize(u_ms_texture);")
                SHADER("    ivec2 tcoord = ivec2(coord * vec2(tsize));")
                SHADER("    int samples = int(factor);")
                SHADER("")
                SHADER("    for (int i = 0; i < samples; ++i)")
                SHADER("        color += texelFetch(sampler, tcoord, i);")
                SHADER("")
                SHADER("    return color / factor;")
                SHADER("}")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    int index = b_index;")
                SHADER("") // Apply clipping
                SHADER("    for (int i=0; i<b_clips; ++i)")
                SHADER("    {")
                SHADER("        vec4 rect = texelFetch(u_buf_commands, index);") // Axis-aligned bound box
                SHADER("        if ((b_frag_coord.x < rect.x) ||")
                SHADER("            (b_frag_coord.y < rect.y) ||")
                SHADER("            (b_frag_coord.x > rect.z) ||")
                SHADER("            (b_frag_coord.y > rect.w))")
                SHADER("            discard;")
                SHADER("        ++index;")
                SHADER("    }")
                SHADER("") // Compute color of fragment
                SHADER("    if (b_coloring == 0)") // Solid color
                SHADER("    {")
                SHADER("        gl_FragColor = texelFetch(u_buf_commands, index);")
                SHADER("    }")
                SHADER("    else if (b_coloring == 1)") // Linear gradient
                SHADER("    {")
                SHADER("        vec4 cs = texelFetch(u_buf_commands, index);")      // Start color
                SHADER("        vec4 ce = texelFetch(u_buf_commands, index + 1);")  // End color
                SHADER("        vec4 gp = texelFetch(u_buf_commands, index + 2);")  // Gradient parameters
                SHADER("        vec2 dv = gp.zw - gp.xy;") // Gradient direction vector
                SHADER("        vec2 dp = b_frag_coord - gp.xy;") // Dot direction vector
                SHADER("        gl_FragColor = mix(cs, ce, clamp(dot(dv, dp) / dot(dv, dv), 0.0f, 1.0f));")
                SHADER("    }")
                SHADER("    else if (b_coloring == 2)") // Radial gradient
                SHADER("    {")
                SHADER("        vec4 cs = texelFetch(u_buf_commands, index);")      // Start color
                SHADER("        vec4 ce = texelFetch(u_buf_commands, index + 1);")  // End color
                SHADER("        vec4 gp = texelFetch(u_buf_commands, index + 2);")  // Gradient parameters: center {xc, yc} and focal {xf, yf}
                SHADER("        vec4 r  = texelFetch(u_buf_commands, index + 3);")  // Radius (R)
                SHADER("        vec2 d  = b_frag_coord.xy - gp.zw;")                // D = {x, y} - {xf, yf }
                SHADER("        vec2 f  = gp.zw - gp.xy;")                          // F = {xf, yf} - {xc, yc }
                SHADER("        float a = dot(d.xy, d.xy);")                        // a = D*D = { Dx*Dx + Dy*Dy }
                SHADER("        float b = 2.0f * dot(f.xy, d.xy);")                 // b = 2*F*D = { 2*Fx*Dx + 2*Fy*Dy }
                SHADER("        float c = dot(f.xy, f.xy) - r.x*r.x;")              // c = F*F = { Fx*Fx + Fy*Fy - R*R }
                SHADER("        float k = (2.0f*a)/(sqrt(b*b - 4.0f*a*c)-b);")      // k = 1/t
                SHADER("        gl_FragColor = mix(cs, ce, clamp(k, 0.0f, 1.0f));")
                SHADER("    }")
                SHADER("    else") // if (b_coloring == 3) Texture-based fill
                SHADER("    {")
                SHADER("        vec4 mc = texelFetch(u_buf_commands, index);")      // Modulating color
                SHADER("        vec4 tp = texelFetch(u_buf_commands, index + 1);")  // Texture parameters: initial size { w, h }, format, multisampling
                SHADER("        vec4 tcolor = (tp.w > 0.5f) ? ")                    // Get color from texture
                SHADER("            textureMultisample(u_ms_texture, b_texcoord, tp.w) :")
                SHADER("            texture(u_texture, b_texcoord);")
                SHADER("        int format = int(tp.z);")                           // Get texture format
                SHADER("        if (format == 0)") // Usual RGBA
                SHADER("            gl_FragColor = vec4(tcolor.rgb * mc.rgb * tcolor.a, tcolor.a * mc.a);")
                SHADER("        else if (format == 1)") // Alpha-blending channel
                SHADER("            gl_FragColor = vec4(mc.rgb * tcolor.r, mc.a * tcolor.r);")
                SHADER("        else") // if (format == 2) // Pre-multiplied RGBA in texture
                SHADER("            gl_FragColor = vec4(tcolor.rgb * mc.rgb, tcolor.a * mc.a);")
                SHADER("    }")
                SHADER("}")
                SHADER("");

            static const char *stencil_vertex_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("uniform mat4 u_model;")
                SHADER("")
                SHADER("in vec2 a_vertex;")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    gl_Position = u_model * vec4(a_vertex, 0.0f, 1.0f);")
                SHADER("}")
                SHADER("");

            static const char *stencil_fragment_shader =
                SHADER("#version 330 core")
                SHADER("")
                SHADER("void main()")
                SHADER("{")
                SHADER("    gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 0.0f);")
                SHADER("}")
                SHADER("");

            #undef SHADER
        } /* namespace glx */
    } /* namespace ws */
} /* namespace lsp */

#endif /* PRIVATE_GL_GLX_SHADERS_H_ */
