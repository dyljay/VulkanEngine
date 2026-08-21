#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_shading_language_include : require

#include "material.glsl"

#define PI 3.14159265359
#define MAX_TEXTURES 100

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

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

layout(set = 1, binding = 0) readonly uniform MaterialProperties {
  // 16 bytes
  vec4 baseColor;

  // 16 bytes
  uint numFeatures;
  uint baseColorMap;
  float metallic;
  float roughness;

  // 16 bytes
  uint normalMap;
  uint occlusionMap;
  uint metallicRoughness;
  uint offset;
} matProperties;

layout(set = 2, binding = 0) uniform samplerCube cubeMap;

layout(set = 3, binding = 0) uniform sampler2D bindlessTextures[MAX_TEXTURES];

float normalDistribution(vec3 n, vec3 h, float a)
{
  float a2 = a * a;
  float n_dot_h = max(dot(n, h), 0.0);
  float n_dot_h2 = n_dot_h * n_dot_h;

  float denomTerm = (n_dot_h2 * (a2 - 1.0) + 1.0);

  float denom = PI * denomTerm * denomTerm;

  return a2 / denom;
}

float geometryGGXSchlick(float nDotV, float k)
{
  return nDotV / (nDotV * (1.0 - k) + k);
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float k)
{
  float nDotV = max(dot(n, v), 0.0);
  float nDotL = max(dot(n, l), 0.0);

  return geometryGGXSchlick(nDotV, k) * geometryGGXSchlick(nDotL, k);
}

vec3 fresnelSchlick(float hDotV, vec3 f0)
{
  return f0 + (1 - f0) * pow(1.0 - hDotV, 5.0);
}

float kDirect(float a)
{
  float num = (a + 1.0);
  float num2 = num * num;

  return num2 / 8.0;
}

float kIBL(float a)
{
  return (a * a) / 2;
}

void main() {
  vec4 albedo;
  if (bool(matProperties.numFeatures & GLSL_HAS_BASE_MAP)) {
    albedo = texture(bindlessTextures[(matProperties.baseColorMap + matProperties.offset)], fragUV);
  }
  else {
    albedo = matProperties.baseColor;
  }

  float roughness = matProperties.roughness;
  float metallic = matProperties.metallic;
  if (bool(matProperties.numFeatures & GLSL_HAS_METAL_MAP)) {
    vec4 metalRough = texture(bindlessTextures[(matProperties.metallicRoughness + matProperties.offset)], fragUV);
    roughness *= metalRough.g;
    metallic *= metalRough.b;
  }
  roughness = clamp(roughness, 0.045, 1.0);
  float alpha = roughness * roughness;

  vec3 ambientLight = vec3(.2);
  vec3 surfaceNormal = normalize(fragNormalWorld);

  vec3 cameraPosWorld = ubo.invView[3].xyz;
  vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

  vec3 f0 = mix(vec3(0.04), albedo.rgb, metallic);
  float nDotV = max(dot(surfaceNormal, viewDirection), 0.0);

  vec3 Lo = vec3(0.0);

  for (int i = 0; i < ubo.numActiveLights; i++) {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);
    directionToLight = normalize(directionToLight);

    float nDotL = max(dot(surfaceNormal, directionToLight), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;

    vec3 halfAngle = normalize(directionToLight + viewDirection);

    float D = normalDistribution(surfaceNormal, halfAngle, alpha);
    float G = geometrySmith(surfaceNormal, viewDirection, directionToLight, kDirect(roughness));
    vec3 F = fresnelSchlick(max(dot(halfAngle, viewDirection), 0.0), f0);

    vec3 specular = (D * G * F) / max(4.0 * nDotV * nDotL, 0.001);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    Lo += (kD * albedo.rgb / PI + specular) * intensity * nDotL;
  }

  vec3 I = normalize(fragPosWorld - cameraPosWorld);
  vec3 R = reflect(I, surfaceNormal);
  vec3 fragSkybox = texture(cubeMap, R).rgb;

  vec3 ambientSpecular = fragSkybox * f0 * (1.0 - roughness);

  vec3 ambient = (ubo.ambientLightColor.xyz * ubo.ambientLightColor.w + ambientLight) * albedo.rgb;

  // total sum
  outColor = vec4(Lo + ambientSpecular + ambient, albedo.a);
}
