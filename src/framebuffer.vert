#version 460 core

layout(location = 0) in vec2 in_coord;

void main() {
  vec2 uv = in_coord / vec2(180.f, 90.f);
  gl_Position = vec4(uv, 0.f, 1.f);
}

