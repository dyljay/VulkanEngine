#include "outline_system.hpp"

#include <cstdint>

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

OutlineSystem::OutlineSystem(
    EngineDevice& device,
    const Shader& shaders,
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    VkDescriptorSet& textureDescriptorSet,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : textureDescriptorSet{textureDescriptorSet},
      RenderSystem(device,
                   shaders,
                   descriptorSetLayouts,
                   sizeof(uint32_t),
                   attachmentInfo)
{}

OutlineSystem::~OutlineSystem() {}

void OutlineSystem::render(FrameInfo& frameInfo)
{
  getPipeline()->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &textureDescriptorSet,
                          0,
                          nullptr);
  OutlinePush push{};
  push.idObj = 1;

  vkCmdPushConstants(frameInfo.commandBuffer,
                     getPipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0,
                     sizeof(OutlinePush),
                     &push);

  vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}
}  // namespace GameEngine
