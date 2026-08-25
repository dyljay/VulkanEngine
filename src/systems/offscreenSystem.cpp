#include "offscreenSystem.hpp"

#include <vector>

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "glm/fwd.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

struct OffscreenPushConstants {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
  VkDeviceAddress vertexBuffer;
  glm::uint32 id;
};

OffscreenSystem::OffscreenSystem(
    EngineDevice& geDevice,
    const Shader& shaders,
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkDescriptorSet>& uboSets,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : uboSets{uboSets},
      RenderSystem(geDevice,
                   shaders,
                   descriptorSetLayouts,
                   sizeof(OffscreenPushConstants),
                   attachmentInfo,
                   {.sampleCount = VK_SAMPLE_COUNT_1_BIT})
{}

OffscreenSystem::~OffscreenSystem() {}

void OffscreenSystem::render(FrameInfo& frameInfo)
{
  getPipeline()->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &uboSets[frameInfo.frameIndex],
                          0,
                          nullptr);

  for (auto& kv : frameInfo.gameObjects) {
    auto& obj = kv.second;
    if (obj.model == nullptr) continue;

    OffscreenPushConstants pushData{};
    pushData.modelMatrix = obj.transform.mat4();
    pushData.normalMatrix = obj.transform.normalMatrix();
    pushData.id = obj.getID();

    for (auto& mesh : obj.model->meshes) {
      pushData.vertexBuffer = mesh->getVertexBufferAddress();
      vkCmdPushConstants(
          frameInfo.commandBuffer,
          getPipelineLayout(),
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          0,
          sizeof(OffscreenPushConstants),
          &pushData);

      mesh->bind(frameInfo.commandBuffer);
      mesh->draw(frameInfo.commandBuffer);
    }
  }
}
}  // namespace GameEngine
