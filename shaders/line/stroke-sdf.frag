#version 330 core

in vec2 v_sdf;
flat in float v_mode;

uniform vec4 u_color;
uniform float u_antialias;
uniform float u_halfWidth;

out vec4 fragColor;

void main() {
    float alpha = 1.0;

    if (v_mode < 0.5) {
        // Core
        alpha = 1.0;
    } else if (v_mode < 1.5) {
        // Linear side / join fringe
        float d = v_sdf.x;
        alpha = 1.0 - smoothstep(0.0, u_antialias, d);
    } else {
        // Round cap fringe
        float capX = max(v_sdf.x, 0.0);
        float sideY = v_sdf.y;

        // Signed distance to a circle of radius halfWidth centered at endpoint
        float d = length(vec2(capX, sideY)) - u_halfWidth;

        alpha = 1.0 - smoothstep(0.0, u_antialias, d);
    }

    fragColor = vec4(u_color.rgb, u_color.a * alpha);
}
