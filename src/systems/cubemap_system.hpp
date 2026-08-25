#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "system.hpp"

namespace GameEngine {

class CubeMapRenderSystem : RenderSystem {
 public:
  CubeMapRenderSystem(
      EngineDevice& device,
      const Shader& shaders,
      const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
      const std::vector<VkDescriptorSet>& uboSets,
      VkDescriptorSet& cubeMap,
      VkPipelineRenderingCreateInfo attachmentInfo);

  ~CubeMapRenderSystem();

  CubeMapRenderSystem(const CubeMapRenderSystem&) = delete;
  CubeMapRenderSystem& operator=(const CubeMapRenderSystem&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  const std::vector<VkDescriptorSet>& uboSets;
  VkDescriptorSet& cubeMap;
};
}  // namespace GameEngine
