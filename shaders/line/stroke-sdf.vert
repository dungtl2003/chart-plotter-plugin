#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_sdf;
layout(location = 2) in float a_mode;

uniform mat4 u_mvp;

out vec2 v_sdf;
flat out float v_mode;

void main() {
    v_sdf = a_sdf;
    v_mode = a_mode;

    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
