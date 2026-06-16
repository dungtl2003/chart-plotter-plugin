#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

uniform mat4 u_mvp;

out vec2 v_texCoord;

void main() {
  v_texCoord = a_texCoord;
  gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
