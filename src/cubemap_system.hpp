#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "src/engine_descriptor.hpp"
#include "src/system.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace GameEngine {

class CubeMapRenderSystem : RenderSystem {
public:
  CubeMapRenderSystem(
      EngineDevice &device, Shader shaders,
      const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
      VkRenderPass renderPass);

  ~CubeMapRenderSystem();

  CubeMapRenderSystem(const CubeMapRenderSystem &) = delete;
  CubeMapRenderSystem &operator=(const CubeMapRenderSystem &) = delete;

  void render(FrameInfo &frameinfo, DescriptorSets &descriptorSets);
};
} // namespace GameEngine
