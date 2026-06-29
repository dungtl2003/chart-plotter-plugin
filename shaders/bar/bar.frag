#version 330 core

in vec2 v_pos;

uniform vec4 u_color;
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

void main() {
    if (!inBound(v_pos, u_plotArea)) {
        discard;
    }

    // Premultiplied alpha to match glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
    fragColor = vec4(u_color.rgb * u_color.a, u_color.a);
}
