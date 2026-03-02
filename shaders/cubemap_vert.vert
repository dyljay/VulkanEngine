#version 450

layout (location = 0) in vec3 inPosition;

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
  mat4 viewNoTranslation = mat4(mat3(ubo.view));

  vec4 position = ubo.projection * viewNoTranslation * inPosition;

  gl_Position = position.xyww;
}
