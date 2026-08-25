#include "bbox_system.hpp"

#include "engine_descriptor.hpp"
#include "engine_frame_info.hpp"
#include "glm/fwd.hpp"
#include "primitive.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

struct BboxPushConstant {
  glm::mat4 modelMatrix;
  AABBPush tlas;
};

BboxRenderer::BboxRenderer(
    EngineDevice& geDevice,
    const Shader& shaders,
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkDescriptorSet>& uboSets,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : uboSets{uboSets},
      RenderSystem{geDevice,
                   shaders,
                   descriptorSetLayouts,
                   sizeof(BboxPushConstant),
                   attachmentInfo,
                   {.polyMode = VK_POLYGON_MODE_LINE}}
{}

BboxRenderer::~BboxRenderer() {}

void BboxRenderer::render(FrameInfo& frameInfo)
{
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &uboSets[frameInfo.frameIndex],
                          0,
                          nullptr);

  getPipeline()->bind(frameInfo.commandBuffer);

  for (auto& kv : frameInfo.gameObjects) {
    auto& obj = kv.second;
    if (obj.model == nullptr) continue;

    BboxPushConstant push{};

    for (auto& mesh : obj.model->meshes) {
      push.modelMatrix = mesh->getLocalMatrix();
      push.tlas.min = mesh->getBBox().min;
      push.tlas.max = mesh->getBBox().max;

      vkCmdPushConstants(
          frameInfo.commandBuffer,
          getPipelineLayout(),
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          0,
          sizeof(BboxPushConstant),
          &push);

      vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
    }
  }
}

}  // namespace GameEngine
