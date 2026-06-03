#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_center;

uniform mat4 u_mvp;

out vec2 v_position;
out vec2 v_center;

void main()
{
    v_position = a_position;
    v_center = a_center;

    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
