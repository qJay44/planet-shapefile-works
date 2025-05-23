#version 460 core

layout(location = 0) in vec2 in_coord;

layout(location = 0) uniform float idx;

void main() {
  vec2 uv = in_coord / vec2(180.f, 90.f);
  uv.x += 0.5f - 1.f * idx;
  uv.x *= 2.f;
  gl_Position = vec4(uv, 0.f, 1.f);
}

