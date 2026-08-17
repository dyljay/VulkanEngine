#pragma once

#include <vector>

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"

namespace GameEngine {

class BboxRenderer : RenderSystem {
 public:
  BboxRenderer(EngineDevice& geDevice,
               Shader shaders,
               const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
               VkPipelineRenderingCreateInfo attachmentInfo);

  ~BboxRenderer();

  BboxRenderer(const BboxRenderer&) = delete;
  BboxRenderer& operator=(const BboxRenderer&) = delete;

  void render(FrameInfo& frameinfo, DescriptorSets& descriptorSets);
};
}  // namespace GameEngine
