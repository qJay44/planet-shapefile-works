#version 460 core

out vec4 FragColor;

in vec2 texCoord;

layout(location = 2) uniform sampler2D fbTexture;

void main() {
  FragColor = vec4(vec3(texture(fbTexture, texCoord).r), 1.f);
}

