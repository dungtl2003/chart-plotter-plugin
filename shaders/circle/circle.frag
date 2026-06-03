#version 330 core

in float v_distance;

uniform vec4 u_color;
uniform float u_radius;
uniform float u_antialias;

out vec4 FragColor;

void main()
{
    float alpha = 1.0 - smoothstep(
        u_radius,
        u_radius + u_antialias,
        v_distance
    );

    FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
