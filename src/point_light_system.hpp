#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_pipeline.hpp"

// std
#include <memory>

namespace GameEngine {
class PointLightSystem {
public:
  PointLightSystem(EngineDevice &device, VkRenderPass renderPass,
                   VkDescriptorSetLayout uboSet);
  ~PointLightSystem();

  PointLightSystem(const PointLightSystem &) = delete;
  PointLightSystem &operator=(const PointLightSystem &) = delete;

  void update(FrameInfo &frameinfo, GlobalUbo &ubo);

  void render(FrameInfo &frameinfo);

private:
  void createPipelineLayout(VkDescriptorSetLayout uboSetLayout);
  void createPipeline(VkRenderPass renderPass);

  EngineDevice &geDevice;
  std::unique_ptr<GraphicsPipeline> gePipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace GameEngine
