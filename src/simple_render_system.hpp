#pragma once

#include "engine_app.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_pipeline.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <memory>

namespace GameEngine {
class SimpleRenderSystem {
public:
  SimpleRenderSystem(EngineDevice &device, VkRenderPass renderPass,
                     DescriptorSetLayouts &setLayout);
  ~SimpleRenderSystem();

  SimpleRenderSystem(const SimpleRenderSystem &) = delete;
  SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

  void renderGameObjects(FrameInfo &frameinfo, DescriptorSets &descriptorSets);

private:
  void createPipelineLayout(DescriptorSetLayouts &setLayouts);
  void createPipeline(VkRenderPass renderPass);

  EngineDevice &geDevice;
  std::unique_ptr<GraphicsPipeline> gePipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace GameEngine
