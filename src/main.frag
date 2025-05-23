#version 460 core

out vec4 FragColor;

in vec2 texCoord;

layout(location = 2) uniform sampler2D fbTexture;
layout(location = 3) uniform vec2 stepUV;

float getTexValue(vec2 uv) {
  return texture(fbTexture, uv).r;
}

void main() {
  float tl      = getTexValue(texCoord + vec2(-stepUV.x, stepUV.y));
  float top     = getTexValue(texCoord + vec2(0.f, stepUV.y));
  float tr      = getTexValue(texCoord + vec2(stepUV.x, stepUV.y));
  float ml      = getTexValue(texCoord + vec2(-stepUV.x, 0.f));
  float middle  = getTexValue(texCoord);
  float mr      = getTexValue(texCoord + vec2(stepUV.x, 0.f));
  float bl      = getTexValue(texCoord + vec2(-stepUV.x, -stepUV.y));
  float bottom  = getTexValue(texCoord + vec2(0.f, -stepUV.y));
  float br      = getTexValue(texCoord + vec2(stepUV.x, -stepUV.y));

  vec3 color = vec3(tl+top+tr+ml+middle+mr+bl+bottom+br);

  FragColor = vec4(color, 1.f);
}

