#version 450

#extension GL_EXT_buffer_reference : require

const int[] array = int[](); 

struct AABB {
  vec3 min;
  float padding0;
  vec3 max;
  float padding1;
}

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int numActiveLights;
} ubo;

layout(buffer_reference, std430) readonly buffer AABBBuffer {
  AABB bbox[];
};

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 bboxBuffer;
} push;


void main() { 
  gl_Position
}
