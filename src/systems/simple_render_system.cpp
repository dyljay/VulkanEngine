#include "simple_render_system.hpp"

#include <cstddef>
#include <vector>

#include "engine_frame_info.hpp"
#include "engine_game_object.hpp"
#include "vulkan/vulkan_core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GameEngine {

struct SimplePushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
  glm::mat4 localTransform{1.f};
  VkDeviceAddress vertexBuffer;
};

SimpleRenderSystem::SimpleRenderSystem(
    EngineDevice& device,
    const Shader& shaders,
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkDescriptorSet>& uboSets,
    std::vector<VkDescriptorSet>& materialSets,
    VkDescriptorSet& cubeMap,
    VkDescriptorSet& textureArray,
    VkPipelineRenderingCreateInfo attachmentInfo)
    : uboSets{uboSets},
      materialSets{materialSets},
      cubeMap{cubeMap},
      textureArray{textureArray},
      RenderSystem{device,
                   shaders,
                   descriptorSetLayouts,
                   sizeof(SimplePushConstantData),
                   attachmentInfo}
{}

SimpleRenderSystem::~SimpleRenderSystem() {}

void SimpleRenderSystem::render(FrameInfo& frameInfo)
{
  getPipeline()->bind(frameInfo.commandBuffer);

  // ubo buffer
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          0,
                          1,
                          &uboSets[frameInfo.frameIndex],
                          0,
                          nullptr);

  // cubemap
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          2,
                          1,
                          &cubeMap,
                          0,
                          nullptr);

  // sample2D array binding
  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(),
                          3,
                          1,
                          &textureArray,
                          0,
                          nullptr);

  std::map<int, bool> visited_materials;
  int materialOffset = 0;

  for (auto& kv : frameInfo.gameObjects) {
    auto& obj = kv.second;
    if (obj.model == nullptr) continue;

    SimplePushConstantData pushData{};
    pushData.normalMatrix = obj.transform.normalMatrix();
    pushData.modelMatrix = obj.transform.mat4();

    for (auto& mesh : obj.model->meshes) {
      pushData.vertexBuffer = mesh->getVertexBufferAddress();
      pushData.localTransform = mesh->getLocalMatrix();

      vkCmdPushConstants(
          frameInfo.commandBuffer,
          getPipelineLayout(),
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          0,
          sizeof(SimplePushConstantData),
          &pushData);

      mesh->bind(frameInfo.commandBuffer);

      for (auto& p : mesh->getSurfaces()) {
        vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                getPipelineLayout(),
                                1,
                                1,
                                &materialSets[p.materialIndex + materialOffset],
                                0,
                                nullptr);

        vkCmdDrawIndexed(frameInfo.commandBuffer,
                         p.count,
                         1,
                         p.startIndex,
                         0,
                         0);

        if (!visited_materials.contains(p.materialIndex + materialOffset)) {
          visited_materials[p.materialIndex + materialOffset] = true;
        }
      }
    }
    materialOffset = visited_materials.size();
  }
}
}  // namespace GameEngine
