#include "system.hpp"
#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <cassert>
#include <cstdint>
#include <vector>

namespace GameEngine {

RenderSystem::RenderSystem(
    EngineDevice &geDevice, Shader shaders,
    const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    uint32_t sizeOfPushConstants, VkRenderPass renderPass)
    : geDevice{geDevice} {
  createPipelineLayout(descriptorSetLayouts, sizeOfPushConstants);
  createPipeline(renderPass, shaders);
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(
    const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    uint32_t pushConstantSize) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = pushConstantSize;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(geDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void RenderSystem::createPipeline(VkRenderPass renderPass, Shader shaders) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  gePipeline = std::make_unique<GraphicsPipeline>(
      geDevice, shaders.vertexShader, shaders.fragShader, pipelineConfig);
}
} // namespace GameEngine
