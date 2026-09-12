#include "outline_system.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_image.hpp"
#include "engine_renderer.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

OutlineSystem::OutlineSystem(
    EngineDevice& device,
    const Shader& shaders,
    EngineDescriptorSetLayout& descriptorSetLayout,
    std::unordered_map<uint32_t, bool>& selectedObjects,
    VkPipelineRenderingCreateInfo pipelineRenderingInfo,
    OffScreenRenderer& offScreenRenderer,
    EngineDescriptorPoolGrowable& growablePool)
    : selectedObjects{selectedObjects},
      RenderSystem(device,
                   shaders,
                   std::vector<VkDescriptorSetLayout>{
                       descriptorSetLayout.getDescriptorSetLayout()},
                   sizeof(uint32_t),
                   pipelineRenderingInfo)
{
  createTextures(device, offScreenRenderer);
  createDescriptorSet(device, descriptorSetLayout, growablePool);
}

void OutlineSystem::createTextures(EngineDevice& device,
                                   OffScreenRenderer& offScreenRenderer)
{
  shaderSamplerImages.resize(offScreenRenderer.getNumberOfImages());

  for (int i = 0; i < offScreenRenderer.getNumberOfImages(); i++) {
    shaderSamplerImages[i] = std::make_unique<EngineTexture>(
        device,
        offScreenRenderer.getImage(i).getImage(),
        offScreenRenderer.getImage(i).getImageView(),
        VK_FILTER_NEAREST,
        VK_SAMPLER_MIPMAP_MODE_NEAREST);
  }
}

void OutlineSystem::createDescriptorSet(
    EngineDevice& device,
    EngineDescriptorSetLayout& setLayout,
    EngineDescriptorPoolGrowable& growablePool)
{
  descriptorSets.resize(shaderSamplerImages.size());

  for (int i = 0; i < descriptorSets.size(); i++) {
    auto imageInfo = shaderSamplerImages[i]->getDescriptorInfo();
    EngineDescriptorWriter(setLayout, growablePool)
        .writeImage(0, &imageInfo)
        .build(descriptorSets[i]);
  }
}

OutlineSystem::~OutlineSystem() {}

void OutlineSystem::render(FrameInfo& frameInfo)
{
  getPipeline()->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &descriptorSets[frameInfo.frameIndex],
                          0,
                          nullptr);

  for (auto& kv : selectedObjects) {
    OutlinePush push{};
    push.idObj = kv.first;
    vkCmdPushConstants(
        frameInfo.commandBuffer,
        getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(OutlinePush),
        &push);

    vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
  }
}
}  // namespace GameEngine
