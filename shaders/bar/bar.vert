#version 330 core

layout(location = 0) in vec2 a_position; // quad corner (screen space)

uniform mat4 u_mvp;

out vec2 v_pos;

void main() {
    v_pos = a_position;
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
