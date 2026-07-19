#version 450

layout(location = 0) flat in uint fragColor;

layout(location = 0) out uint outColor;

void main() {
  outColor = fragColor;
}
