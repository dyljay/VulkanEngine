#include "simple_render_system.hpp"
#include "vulkan/vulkan_core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>

namespace GameEngine {

struct SimplePushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
  VkDeviceAddress vertexBuffer;
};

SimpleRenderSystem::SimpleRenderSystem(EngineDevice &device,
                                       VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout)
    : geDevice{device} {
  createPipelineLayout(globalSetLayout);
  createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(
    VkDescriptorSetLayout globalSetLayout) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

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

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  gePipeline = std::make_unique<GraphicsPipeline>(
      geDevice, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
  gePipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.globalDescriptorSet, 0, nullptr);

  for (auto &kv : frameInfo.gameObjects) {
    auto &obj = kv.second;
    if (obj.model == nullptr)
      continue;
    SimplePushConstantData pushData{};
    pushData.modelMatrix = obj.transform.mat4();
    pushData.normalMatrix = obj.transform.normalMatrix();

    // TODO: maybe find a safer way of getting the individual meshes to render
    // instead of grabbing the vector directly but also this would be the point
    // of a shared pointer

    for (auto &mesh : obj.model->meshes) {
      pushData.vertexBuffer = mesh->getVertexBufferAddress();
      vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT |
                             VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(SimplePushConstantData), &pushData);

      mesh->bind(frameInfo.commandBuffer);
      mesh->draw(frameInfo.commandBuffer);
    }
  }
}

} // namespace GameEngine
