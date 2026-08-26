#version 450

#extension GL_EXT_buffer_reference : require

layout(location = 0) flat out uint fragColor;

struct PointLight {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor;
  PointLight pointLights[10];
  int numActiveLights;
} ubo;

struct Vertex {
  vec3 position;
  float uv_x;
  vec3 normal;
  float uv_y;
  vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 localTransform;
  VertexBuffer vertexBuffer;
  uint id;
} push;

void main() {
  Vertex v = push.vertexBuffer.vertices[gl_VertexIndex];
  vec4 positionWorld = push.modelMatrix * push.localTransform * vec4(v.position, 1.0);
  gl_Position = ubo.projection * ubo.view * positionWorld;

  fragColor = push.id;
}
