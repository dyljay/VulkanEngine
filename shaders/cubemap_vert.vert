#version 450

// Vertex shader — no vertex buffer needed at all
const vec3 positions[36] = vec3[](
    // +X face
    vec3( 1, -1, -1), vec3( 1, -1,  1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3( 1,  1, -1), vec3( 1, -1, -1),
    // -X face
    vec3(-1, -1,  1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3(-1,  1, -1), vec3(-1,  1,  1), vec3(-1, -1,  1),
    // +Y face
    vec3(-1,  1, -1), vec3( 1,  1, -1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3(-1,  1,  1), vec3(-1,  1, -1),
    // -Y face
    vec3(-1, -1,  1), vec3( 1, -1,  1), vec3( 1, -1, -1),
    vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, -1,  1),
    // +Z face
    vec3(-1, -1,  1), vec3(-1,  1,  1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1),
    // -Z face
    vec3( 1, -1, -1), vec3( 1,  1, -1), vec3(-1,  1, -1),
    vec3(-1,  1, -1), vec3(-1, -1, -1), vec3( 1, -1, -1)
);

struct PointLight {
    vec4 position;
    vec4 color;
};

layout(location = 0) out vec3 outDirection;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int numActiveLights;
} ubo;

void main() 
{  
  vec3 pos = positions[gl_VertexIndex];
  mat4 viewNoTranslation = mat4(mat3(ubo.view));
  vec4 position = ubo.projection * viewNoTranslation * vec4(pos, 1.0);

  gl_Position = position.xyww;

  outDirection = normalize(pos);
}
