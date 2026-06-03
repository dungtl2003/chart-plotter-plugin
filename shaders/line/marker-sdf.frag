#version 330 core

in float v_sdf;

uniform vec4 u_color;
uniform float u_antialias;

out vec4 FragColor;

void main()
{
    float alpha = 1.0 - smoothstep(0.0, u_antialias, v_sdf);
    FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
