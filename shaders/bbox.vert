#version 450

#extension GL_EXT_buffer_reference : require

const vec3 positions[8] = vec3[](
    // +X face
    vec3(1, -1, -1), vec3(1, -1, 1), vec3(1, 1, 1),
    vec3(1, 1, 1), vec3(1, 1, -1), vec3(1, -1, -1),
    // -X face
    vec3(-1, -1, 1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(-1, 1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1),
    // +Y face
    vec3(-1, 1, -1), vec3(1, 1, -1), vec3(1, 1, 1),
    vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1),
    // -Y face
    vec3(-1, -1, 1), vec3(1, -1, 1), vec3(1, -1, -1),
    vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, -1, 1),
    // +Z face
    vec3(-1, -1, 1), vec3(-1, 1, 1), vec3(1, 1, 1),
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1),
    // -Z face
    vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1),
    vec3(-1, 1, -1), vec3(-1, -1, -1), vec3(1, -1, -1)
  );

struct PointLight {
  vec4 position;
  vec4 color;
};

struct AABB {
  vec3 min;
  float padding0;
  vec3 max;
  float padding1;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor;
  PointLight pointLights[10];
  int numActiveLights;
} ubo;

layout(push_constant) uniform Push {
  mat4 model;
  AABB tlas;
} push;

layout(location = 0) out vec4 outPositionLin;

void main() {
  vec4 position = vec4(positions[gl_VertexIndex], 1.0);

  position.x = (sign(position.x) == 1.0) ? push.tlas.min.x : push.tlas.max.x;
  position.y = (sign(position.y) == 1.0) ? push.tlas.min.y : push.tlas.max.y;
  position.x = (sign(position.z) == 1.0) ? push.tlas.min.z : push.tlas.max.z;

  vec4 position_mat = ubo.projection * ubo.view * push.model * position;

  outPositionFlat = position;
  outPositionLin = position;
  gl_Position = position;
}
