#include "cubemap_system.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>

namespace GameEngine {

CubeMapRenderSystem::CubeMapRenderSystem(EngineDevice &device,
                                         VkRenderPass renderPass,
                                         VkDescriptorSetLayout uboSetLayout,
                                         VkDescriptorSetLayout cubeMapLayout)
    : geDevice{device} {
  createPipelineLayout(uboSetLayout, cubeMapLayout);
  createPipeline(renderPass);
}

CubeMapRenderSystem::~CubeMapRenderSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void CubeMapRenderSystem::createPipelineLayout(
    VkDescriptorSetLayout uboSetLayout, VkDescriptorSetLayout cubeMapLayout) {

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{uboSetLayout,
                                                          cubeMapLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

  if (vkCreatePipelineLayout(geDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void CubeMapRenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
  // because it's the skybox, this pipline should be <=
  pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.attributeDescriptions.clear();
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  gePipeline = std::make_unique<GraphicsPipeline>(
      geDevice, "shaders/cubemap_vert.vert.spv",
      "shaders/cubemap_frag.frag.spv", pipelineConfig);
}

void CubeMapRenderSystem::renderSkybox(FrameInfo &frameInfo,
                                       VkDescriptorSet &cubeMapSet) {
  gePipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.globalDescriptorSet, 0, nullptr);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                          &cubeMapSet, 0, nullptr);

  vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
}

} // namespace GameEngine
