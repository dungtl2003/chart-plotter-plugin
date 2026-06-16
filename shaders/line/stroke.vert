#version 330 core

layout(location = 0) in vec2 a_position;  // expanded quad corner (screen space)
layout(location = 1) in vec2 a_p0;        // capsule start (screen space)
layout(location = 2) in vec2 a_p1;        // capsule end   (screen space)

uniform mat4 u_mvp;

out vec2 v_pos;
out vec2 v_p0;
out vec2 v_p1;

void main() {
    v_pos = a_position;
    v_p0  = a_p0;
    v_p1  = a_p1;
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
