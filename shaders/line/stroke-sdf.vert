#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in float a_sdf;

uniform mat4 u_mvp;

out float v_sdf;

void main()
{
    v_sdf = a_sdf;
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
