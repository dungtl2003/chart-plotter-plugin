#version 330 core

in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec4 u_color;
                           
out vec4 fragColor;

void main() {
  float coverage = texture(u_texture, v_texCoord).a;
  // premultiplied output -> pairs with glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
  fragColor = vec4(u_color.rgb, 1.0) * (coverage * u_color.a);
}
