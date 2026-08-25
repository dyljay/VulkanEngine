#pragma once

#include <cstdint>
#include <vector>

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "system.hpp"

namespace GameEngine {

struct OutlinePush {
  uint32_t idObj;
};

class OutlineSystem : RenderSystem {
 public:
  OutlineSystem(EngineDevice& device,
                const Shader& shaders,
                const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                VkDescriptorSet& textureDescriptorSet,
                VkPipelineRenderingCreateInfo attachmentInfo);

  ~OutlineSystem();

  OutlineSystem(const OutlineSystem&) = delete;
  OutlineSystem& operator=(const OutlineSystem&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  VkDescriptorSet& textureDescriptorSet;
};

}  // namespace GameEngine
