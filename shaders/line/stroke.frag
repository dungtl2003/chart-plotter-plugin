#version 330 core

in vec2 v_sdf;

uniform vec4 u_color;

out vec4 fragColor;

void main() {
    fragColor = u_color;
}
