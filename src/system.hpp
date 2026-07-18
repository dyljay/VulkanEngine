#pragma once

#include "engine_device.hpp"
#include "engine_pipeline.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {

struct Shader {
  std::string vertexShader;
  std::string fragShader;
};

class RenderSystem {
public:
  RenderSystem(EngineDevice &geDevice, Shader shaders,
               const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
               uint32_t sizeOfPushConstants, VkRenderPass renderPass);

  ~RenderSystem();

  virtual void render() = 0;

private:
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
      uint32_t pushConstantSize);

  void createPipeline(VkRenderPass renderPass, Shader shaders);

  EngineDevice &geDevice;
  std::unique_ptr<GraphicsPipeline> gePipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace GameEngine
