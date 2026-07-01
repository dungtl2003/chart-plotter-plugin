#version 330 core

in vec4 v_color;

out vec4 fragColor;

void main() {
    // Premultiplied alpha to match glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
    fragColor = vec4(v_color.rgb * v_color.a, v_color.a);
}
