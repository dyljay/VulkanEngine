#pragma once

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_pipeline.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {

struct Shader {
  const std::string &vertexShader;
  const std::string &fragShader;
};

struct PipeLineSettings {
  bool clearDescriptions = false;
  VkCompareOp comparison = VK_COMPARE_OP_LESS;
};
class RenderSystem {
public:
  RenderSystem(EngineDevice &geDevice, const Shader shaders,
               const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
               uint32_t sizeOfPushConstants, VkRenderPass renderPass,
               const PipeLineSettings &pipelineSettings = {});

  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  virtual void render(FrameInfo &frameInfo, DescriptorSets &descriptorSets) = 0;

protected:
  std::unique_ptr<GraphicsPipeline> &getPipeline() { return gePipeline; }

  VkPipelineLayout &getPipelineLayout() { return pipelineLayout; }

private:
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
      uint32_t pushConstantSize);

  void createPipeline(VkRenderPass renderPass, Shader shaders,
                      const PipeLineSettings &pipelineSettings);

  EngineDevice &geDevice;
  std::unique_ptr<GraphicsPipeline> gePipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace GameEngine
