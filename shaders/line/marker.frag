#version 330 core

in vec2 v_pos;
in vec2 v_center;

uniform vec4 u_color;
uniform float u_radius;
uniform float u_antialias;

out vec4 fragColor;

float sdCircle(vec2 p, vec2 center, float radius) {
    return distance(p, center) - radius;
}

void main() {
    float dist = sdCircle(v_pos, v_center, u_radius);

    float aa = max(u_antialias, fwidth(dist));
    float alpha = 1.0 - smoothstep(0.0, aa, dist);

    if (alpha <= 0.0) discard;
    fragColor = vec4(u_color.rgb, u_color.a * alpha);
}
