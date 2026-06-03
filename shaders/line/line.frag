#version 330 core

in float v_distance;

uniform vec4 u_color;
uniform float u_halfWidth;
uniform float u_antialias;

out vec4 FragColor;

void main()
{
    float d = abs(v_distance);

    float alpha = 1.0 - smoothstep(
        u_halfWidth,
        u_halfWidth + u_antialias,
        d
    );

    FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
