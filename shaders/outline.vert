#version 450

layout(set = 0, binding = 0) uniform usampler2D textureID;

layout(push_constant) uniform Push {
  uint objectID;
};

void main() {
  vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
