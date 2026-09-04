#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform usampler2D textureID;

layout(push_constant) uniform Push {
  uint selectedID;
} push;

void main() {
  for (int i = -1; i < 2; i++) {
    for (int j = -1; j < 2; j++) {
      vec4 uv = texelFetch(textureID, ivec2(gl_FragCoord.x + i, gl_FragCoord.y + j), 0);
      if (uv.x != push.selectedID) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
      }
    }
  }

  discard;
}
