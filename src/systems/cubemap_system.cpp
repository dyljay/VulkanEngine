#include "cubemap_system.hpp"

#include <cstddef>
#include <vector>

#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "src/engine_descriptor.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

CubeMapRenderSystem::CubeMapRenderSystem(
    EngineDevice& device,
    const Shader& shaders,
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkDescriptorSet>& uboSets,
    VkDescriptorSet& cubeMap,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : uboSets{uboSets},
      cubeMap{cubeMap},
      RenderSystem(device,
                   shaders,
                   descriptorSetLayouts,
                   0,
                   attachmentInfo,
                   {.clearDescriptions = true,
                    .comparison = VK_COMPARE_OP_LESS_OR_EQUAL})
{}

CubeMapRenderSystem::~CubeMapRenderSystem() {}

void CubeMapRenderSystem::render(FrameInfo& frameInfo)
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

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          1,
                          1,
                          &cubeMap,
                          0,
                          nullptr);

  vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
}

}  // namespace GameEngine
