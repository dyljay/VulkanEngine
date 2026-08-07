#pragma once

#include <vector>

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

class OffscreenSystem : RenderSystem {
 public:
  OffscreenSystem(EngineDevice& geDevice,
                  Shader shaders,
                  const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
                  VkPipelineRenderingCreateInfo attachmentInfo);
  ~OffscreenSystem();

  OffscreenSystem(const OffscreenSystem&) = delete;
  OffscreenSystem& operator=(const OffscreenSystem&) = delete;

  void render(FrameInfo& frameinfo, DescriptorSets& DescriptorSets);
};
}  // namespace GameEngine
