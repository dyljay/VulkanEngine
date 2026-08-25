#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "system.hpp"

// std
#include <vector>

namespace GameEngine {
class SimpleRenderSystem : RenderSystem {
 public:
  SimpleRenderSystem(
      EngineDevice& device,
      const Shader& shaders,
      const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
      const std::vector<VkDescriptorSet>& uboSets,
      std::vector<VkDescriptorSet>& materialSets,
      VkDescriptorSet& cubeMap,
      VkDescriptorSet& textureArray,
      VkPipelineRenderingCreateInfo attachmentInfo);

  ~SimpleRenderSystem();

  SimpleRenderSystem(const SimpleRenderSystem&) = delete;
  SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  const std::vector<VkDescriptorSet>& uboSets;
  std::vector<VkDescriptorSet>& materialSets;
  VkDescriptorSet& cubeMap;
  VkDescriptorSet& textureArray;
};
}  // namespace GameEngine
