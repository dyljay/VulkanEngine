#include "cubemap_system.hpp"
#include "src/engine_descriptor.hpp"
#include "src/system.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <vector>

namespace GameEngine {

CubeMapRenderSystem::CubeMapRenderSystem(
    EngineDevice &device, Shader shaders,
    const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    VkRenderPass renderPass)
    : RenderSystem(device, shaders, descriptorSetLayouts, 0, renderPass,
                   {.clearDescriptions = true,
                    .comparison = VK_COMPARE_OP_LESS_OR_EQUAL}) {}

CubeMapRenderSystem::~CubeMapRenderSystem() {}

void CubeMapRenderSystem::render(FrameInfo &frameInfo,
                                 DescriptorSets &descriptorSet) {
  getPipeline()->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                          0, 1, &descriptorSet.uboSets[frameInfo.frameIndex], 0,
                          nullptr);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                          1, 1, &descriptorSet.cubeMap, 0, nullptr);

  vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
}

} // namespace GameEngine
