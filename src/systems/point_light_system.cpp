#include "point_light_system.hpp"

#include <vector>

#include "src/engine_descriptor.hpp"
#include "vulkan/vulkan_core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GameEngine {

struct PointLightPushConstants {
  glm::vec4 position{};
  glm::vec4 color{};
  float radius;
};

PointLightSystem::PointLightSystem(
    EngineDevice& device,
    Shader shaders,
    const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : RenderSystem(device,
                   shaders,
                   descriptorSetLayouts,
                   sizeof(PointLightPushConstants),
                   attachmentInfo,
                   {.clearDescriptions = true})
{}

PointLightSystem::~PointLightSystem() {}

void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo)
{
  auto rotateLight =
      glm::rotate(glm::mat4(1.0f), frameInfo.frameTime, {0.f, -1.f, 0.f});

  int lightIndex = 0;
  for (auto& kv : frameInfo.gameObjects) {
    auto& obj = kv.second;
    if (obj.pointLight == nullptr) continue;

    assert(lightIndex < MAX_LIGHTS &&
           "Point Lights exceed maximum number specified");

    // update light object
    obj.transform.translation =
        glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));

    ubo.pointLights[lightIndex].position =
        glm::vec4(obj.transform.translation, 1.f);
    ubo.pointLights[lightIndex].color =
        glm::vec4(obj.color, obj.pointLight->lightIntensity);

    lightIndex += 1;
  }

  ubo.numActiveLights = lightIndex;
}

void PointLightSystem::render(FrameInfo& frameInfo,
                              DescriptorSets& descriptorSets)
{
  getPipeline()->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &descriptorSets.uboSets[frameInfo.frameIndex],
                          0,
                          nullptr);

  for (auto& kv : frameInfo.gameObjects) {
    auto& obj = kv.second;
    if (obj.pointLight == nullptr) continue;

    PointLightPushConstants push{};
    push.position = glm::vec4(obj.transform.translation, 1.f);
    push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
    push.radius = obj.transform.scale.x;

    vkCmdPushConstants(
        frameInfo.commandBuffer,
        getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PointLightPushConstants),
        &push);

    vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
  }
}

}  // namespace GameEngine
