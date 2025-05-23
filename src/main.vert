#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex;

out vec2 texCoord;

layout(location = 0) uniform mat4 translateMat;
layout(location = 1) uniform mat4 scaleMat;

void main() {
  vec3 pos = vec3(translateMat * scaleMat * vec4(in_pos, 1.f));
  texCoord = in_tex;

  gl_Position = vec4(pos, 1.f);
}

