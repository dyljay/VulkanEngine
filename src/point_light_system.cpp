#include "point_light_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>

namespace GameEngine {

struct PointLightPushConstants {
  glm::vec4 position{};
  glm::vec4 color{};
  float radius;
};

PointLightSystem::PointLightSystem(EngineDevice &device,
                                   VkRenderPass renderPass,
                                   VkDescriptorSetLayout uboSetLayout)
    : geDevice{device} {
  createPipelineLayout(uboSetLayout);
  createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem() {
  vkDestroyPipelineLayout(geDevice.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(
    VkDescriptorSetLayout uboSetLayout) {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PointLightPushConstants);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{uboSetLayout};

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

void PointLightSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  GraphicsPipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.attributeDescriptions.clear();
  pipelineConfig.bindingDescriptions.clear();
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  gePipeline = std::make_unique<GraphicsPipeline>(
      geDevice, "shaders/point_light.vert.spv", "shaders/point_light.frag.spv",
      pipelineConfig);
}

void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
  auto rotateLight =
      glm::rotate(glm::mat4(1.0f), frameInfo.frameTime, {0.f, -1.f, 0.f});

  int lightIndex = 0;
  for (auto &kv : frameInfo.gameObjects) {
    auto &obj = kv.second;
    if (obj.pointLight == nullptr)
      continue;

    assert(lightIndex < MAX_LIGHTS &&
           "Point Lights exceed maximum number specified");

    // update light object
    obj.transform.translation =
        glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));

    ubo.pointLights[lightIndex].position =
        glm::vec4(obj.transform.translation, 1.f);
    ubo.pointLights[lightIndex].color =
        glm::vec4(obj.color, obj.pointLight->lightIntensity);

    lightIndex += 1;
  }

  ubo.numActiveLights = lightIndex;
}

void PointLightSystem::render(FrameInfo &frameInfo) {
  gePipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.globalDescriptorSet, 0, nullptr);

  for (auto &kv : frameInfo.gameObjects) {
    auto &obj = kv.second;
    if (obj.pointLight == nullptr)
      continue;

    PointLightPushConstants push{};
    push.position = glm::vec4(obj.transform.translation, 1.f);
    push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
    push.radius = obj.transform.scale.x;

    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PointLightPushConstants), &push);

    vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
  }
}

} // namespace GameEngine
