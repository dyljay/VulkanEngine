#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

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
  vec3 color;
  vec3 normal;
  vec2 uv;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
  Vertex vertices[];
};

layout( push_constant ) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(inPosition, 1.0);
    gl_Position = ubo.projection * ubo.view * positionWorld;
    
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}
