#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <vector>

namespace GameEngine {
class SimpleRenderSystem : RenderSystem {
public:
  SimpleRenderSystem(
      EngineDevice &device, Shader shaders,
      const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
      VkRenderPass renderPass);

  ~SimpleRenderSystem();

  SimpleRenderSystem(const SimpleRenderSystem &) = delete;
  SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

  void render(FrameInfo &frameinfo, DescriptorSets &descriptorSets);
};
} // namespace GameEngine
