#include "simple_render_system.hpp"
#include "src/engine_app.hpp"
#include "src/engine_frame_info.hpp"
#include "src/engine_game_object.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <stdexcept>

namespace GameEngine {

struct SimplePushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
  VkDeviceAddress vertexBuffer;
};

SimpleRenderSystem::SimpleRenderSystem(EngineDevice &device,
                                       VkRenderPass renderPass,
                                       DescriptorSetLayouts &globalSetLayout)
    : geDevice{device} {
  createPipelineLayout(globalSetLayout);
  createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(DescriptorSetLayouts &setLayout) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
      setLayout.uboSetLayout->getDescriptorSetLayout(),
      setLayout.materialSetLayout->getDescriptorSetLayout(),
      setLayout.cubemap->getDescriptorSetLayout(),
      setLayout.textureLayout->getDescriptorSetLayout()};

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

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo,
                                           DescriptorSets &descriptorSets) {
  gePipeline->bind(frameInfo.commandBuffer);

  // ubo buffer
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.globalDescriptorSet, 0, nullptr);

  // cubemap
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                          &descriptorSets.cubeMap, 0, nullptr);

  // sample2D array binding
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                          &descriptorSets.textureArray, 0, nullptr);

  std::map<int, bool> visited_materials;
  int materialOffset = 0;

  for (auto &kv : frameInfo.gameObjects) {
    auto &obj = kv.second;
    if (obj.model == nullptr)
      continue;

    SimplePushConstantData pushData{};
    pushData.modelMatrix = obj.transform.mat4();
    pushData.normalMatrix = obj.transform.normalMatrix();

    for (auto &mesh : obj.model->meshes) {
      pushData.vertexBuffer = mesh->getVertexBufferAddress();
      vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT |
                             VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(SimplePushConstantData), &pushData);

      mesh->bind(frameInfo.commandBuffer);

      for (auto &p : mesh->getSurfaces()) {
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 1, 1,
            &descriptorSets.materialSets[p.materialIndex + materialOffset], 0,
            nullptr);

        vkCmdDrawIndexed(frameInfo.commandBuffer, p.count, 1, p.startIndex, 0,
                         0);

        if (!visited_materials.contains(p.materialIndex + materialOffset)) {
          visited_materials[p.materialIndex + materialOffset] = true;
        }
      }
    }
    materialOffset += visited_materials.size();
  }
}
} // namespace GameEngine
