#version 330 core

in vec2 v_pos;
in vec2 v_p0;
in vec2 v_p1;

uniform vec4  u_color;
uniform float u_halfWidth;   // line half-width, pixels
uniform float u_antialias;   // feather width, pixels

out vec4 fragColor;

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float denom = max(dot(ba, ba), 1e-6);          // guards the dot case (a == b)
    float h = clamp(dot(pa, ba) / denom, 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    float dist = sdSegment(v_pos, v_p0, v_p1) - u_halfWidth;

    // at least 1px of feather regardless of zoom, then fade outward over `aa`
    float aa = max(u_antialias, fwidth(dist));
    float alpha = 1.0 - smoothstep(0.0, aa, dist);

    if (alpha <= 0.0) discard;
    fragColor = vec4(u_color.rgb, u_color.a * alpha);
}
