#version 330 core

layout(location = 0) in vec2 a_position; // wedge vertex (screen space)
layout(location = 1) in vec4 a_color;    // straight RGBA

uniform mat4 u_mvp;

out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
