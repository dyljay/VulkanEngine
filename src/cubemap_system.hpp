#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_pipeline.hpp"
#include "src/engine_texture.hpp"
#include <vulkan/vulkan_core.h>

namespace GameEngine {

class CubeMapRenderSystem {
public:
  CubeMapRenderSystem(EngineDevice &device, VkRenderPass renderPass,
                      VkDescriptorSetLayout globalSetLayout);
  ~CubeMapRenderSystem();

  CubeMapRenderSystem(const CubeMapRenderSystem &) = delete;
  CubeMapRenderSystem &operator=(const CubeMapRenderSystem &) = delete;

  void renderSkybox(FrameInfo &frameinfo, EngineTexture &texture);

private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
  void createPipeline(VkRenderPass renderPass);

  EngineDevice &geDevice;
  std::unique_ptr<GraphicsPipeline> gePipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace GameEngine
