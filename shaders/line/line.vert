#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in float a_distance;

uniform mat4 u_mvp;

out float v_distance;

void main()
{
    v_distance = a_distance;
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
