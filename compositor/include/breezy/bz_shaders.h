#ifndef BZ_SHADERS_H
#define BZ_SHADERS_H
// #################################################################################################

static constexpr char CLIENT_VERT_SRC[] =
"#version 320 es\n"
"layout(location = 0) in highp vec2 a_vertCoord;\n"
"layout(location = 1) in lowp vec2 a_texCoord;\n"
"uniform mat3 u_outputProj;\n"
"uniform mat3 u_surfaceProj;\n"
"out lowp vec2 v_texCoord;\n"
"void main() {\n"
"    vec3 p = u_outputProj * u_surfaceProj * vec3(a_vertCoord, 1.0);\n"
"	 gl_Position = vec4(p.xy, 0.0, 1.0);\n"
"	 v_texCoord = a_texCoord;\n"
"}\n";

static constexpr char CLIENT_FRAG_SRC[] =
"#version 320 es\n"
"in lowp vec2 v_texCoord;\n"
"uniform sampler2D u_texture;\n"
"out lowp vec4 fragColor;\n"
"void main() {\n"
"	 fragColor = texture(u_texture, v_texCoord);\n"
"}\n";

// #################################################################################################
#endif
