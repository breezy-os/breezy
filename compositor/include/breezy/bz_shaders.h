#ifndef BZ_SHADERS_H
#define BZ_SHADERS_H
// #################################################################################################

static constexpr char CLIENT_VERT_SRC[] =
"#version 320 es\n"
"layout(location = 0) in highp vec2 a_position;\n"
"layout(location = 1) in lowp vec3 a_color;\n"
"out lowp vec3 v_color;\n"
"void main() {\n"
"	gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
"	v_color = a_color;\n"
"}\n";

static constexpr char CLIENT_FRAG_SRC[] =
"#version 320 es\n"
"in lowp vec3 v_color;\n"
"out lowp vec3 fragColor;\n"
"void main() {\n"
"	fragColor = v_color;\n"
"}\n";

// #################################################################################################
#endif
