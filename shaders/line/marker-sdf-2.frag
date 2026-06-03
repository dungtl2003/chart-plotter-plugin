#version 330 core

in vec2 v_position;
in vec2 v_center;

uniform vec4 u_color;
uniform float u_radius;
uniform float u_antialias;

out vec4 FragColor;

void main()
{
    float sdf = length(v_position - v_center) - u_radius;

    float alpha = 1.0 - smoothstep(
        0.0,
        u_antialias,
        sdf
    );

    FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
