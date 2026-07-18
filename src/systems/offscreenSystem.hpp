#pragma once

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"
#include <vector>

namespace GameEngine {

class OffscreenSystem : RenderSystem {
public:
  OffscreenSystem(EngineDevice &geDevice, Shader shaders,
                  const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
                  VkRenderPass renderPass);
  ~OffscreenSystem();

  OffscreenSystem(const OffscreenSystem &) = delete;
  OffscreenSystem &operator=(const OffscreenSystem &) = delete;

  void render(FrameInfo &frameinfo, DescriptorSets &DescriptorSets);
};
} // namespace GameEngine
