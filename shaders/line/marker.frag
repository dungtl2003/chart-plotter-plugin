#version 330 core

in vec2 v_pos;
in vec2 v_center;

uniform vec4 u_color;
uniform float u_radius;
uniform float u_antialias;
uniform vec4 u_plotArea;

out vec4 fragColor;

bool inBound(vec2 p, vec4 boundary) {
    float minX = min(boundary.x, boundary.y);
    float maxX = max(boundary.x, boundary.y);
    float minY = min(boundary.z, boundary.w);
    float maxY = max(boundary.z, boundary.w);
    
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

float sdCircle(vec2 p, vec2 center, float radius) {
    return distance(p, center) - radius;
}

void main() {
    if(!inBound(v_pos, u_plotArea)) {
        discard;
    }

    float dist = sdCircle(v_pos, v_center, u_radius);

    float aa = max(u_antialias, fwidth(dist));
    float alpha = 1.0 - smoothstep(0.0, aa, dist);

    if (alpha <= 0.0) discard;
    // Premultiplied output to match the premultiplied-over blend
    // (GL_ONE, GL_ONE_MINUS_SRC_ALPHA); see line/stroke.frag for why.
    float a = u_color.a * alpha;
    fragColor = vec4(u_color.rgb * a, a);
}
