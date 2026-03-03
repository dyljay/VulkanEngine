#version 450

layout (location = 0) in vec3 textureDirection;

layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform samplerCube cubeMap;

void main() 
{
  outColor = texture(cubeMap, textureDirection); 
}
