#version 330 core

in vec2 v_pos;
in vec2 v_p0;
in vec2 v_p1;

uniform vec4  u_color;
uniform float u_halfWidth;   // line half-width, pixels
uniform float u_antialias;   // feather width, pixels
// vec4(left, right, top, bottom)
uniform vec4 u_plotArea;

out vec4 fragColor;

bool inBound(vec2 p, vec4 boundary) {
    float minX = min(boundary.x, boundary.y);
    float maxX = max(boundary.x, boundary.y);
    float minY = min(boundary.z, boundary.w);
    float maxY = max(boundary.z, boundary.w);
    
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float denom = max(dot(ba, ba), 1e-6);          // guards the dot case (a == b)
    float h = clamp(dot(pa, ba) / denom, 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    if(!inBound(v_pos, u_plotArea)) {
        discard;
    }

    float dist = sdSegment(v_pos, v_p0, v_p1) - u_halfWidth;

    // at least 1px of feather regardless of zoom, then fade outward over `aa`
    float aa = max(u_antialias, fwidth(dist));
    float alpha = 1.0 - smoothstep(0.0, aa, dist);

    if (alpha <= 0.0) discard;
    // Premultiplied output to match the framebuffer's premultiplied-over blend
    // (GL_ONE, GL_ONE_MINUS_SRC_ALPHA). Without this, one segment's AA fringe
    // (a < 1) drawn over a neighbouring segment's already-opaque body composites
    // as C + (1-a)*C instead of back to C, brightening the overlap into a
    // visible rounded seam at every joint.
    float a = u_color.a * alpha;
    fragColor = vec4(u_color.rgb * a, a);
}
