#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform usampler2D textureID;

layout(push_constant) uniform Push {
  uint selectedID;
} push;

void main() {
  vec4 uv = texelFetch(textureID, ivec2(gl_FragCoord.xy), 0);
  outColor = uv;
}
