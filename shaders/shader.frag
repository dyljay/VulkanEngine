#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

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

layout (binding = 1) uniform samplerCube cubeMap;

layout(set = 1, binding = 0) uniform sampler2D bindlessTextures[];

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

#define PI 3.14159265359

void main() {
    vec3 ambientLight = vec3(.2);
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);
    
    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
    
    for (int i = 0 ; i < ubo.numActiveLights ; i++) {
        // diffuse lighting
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);
        
        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;
        
        diffuseLight += intensity * cosAngIncidence;
        
        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 32.0);
        
        specularLight += light.color.xyz * attenuation * blinnTerm;
    }        

    // reflection of cubemap 
    vec3 I = normalize(fragPosWorld - cameraPosWorld);
    vec3 R = reflect(I, normalize(fragNormalWorld));
    vec4 fragSkybox = texture(cubeMap, R);
    
    // ambient specular component from cubemap
    specularLight = specularLight + fragSkybox.rgb;
    
    // total sum
    outColor = fragColor * vec4(diffuseLight + specularLight + ambientLight, 1.0);
}

float normalDistribution(vec3 n, vec3 h, float a) 
{
  float a2 = a*a;
  float n_dot_h = max(dot(n,h), 0.0);
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
  float nDotV = max(dot(n,v), 0.0);
  float nDotL = max(dot(n,l), 0.0);

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

