#include "cubemap_system.hpp"
#include "src/engine_texture.hpp"

namespace GameEngine {

CubeMapRenderSystem::CubeMapRenderSystem(EngineDevice &device,
                                         VkRenderPass renderPass,
                                         VkDescriptorSetLayout globalSetLayout)
    : geDevice{device} {
  createPipelineLayout(globalSetLayout);
  createPipeline(renderPass);
}

CubeMapRenderSystem::~CubeMapRenderSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void CubeMapRenderSystem::createPipelineLayout(
    VkDescriptorSetLayout globalSetLayout) {

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

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
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.attributeDescriptions.clear();
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  gePipeline = std::make_unique<GraphicsPipeline>(
      geDevice, "shaders/cubemap_vert.vert.spv",
      "shaders/cubemap_frag.frag.spv", pipelineConfig);
}

void CubeMapRenderSystem::renderSkybox(FrameInfo &frameInfo,
                                       EngineTexture &texture) {
  gePipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.globalDescriptorSet, 0, nullptr);

  texture.drawCubeMap(frameInfo.commandBuffer);
}

} // namespace GameEngine
