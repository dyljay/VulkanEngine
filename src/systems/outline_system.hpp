#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_texture.hpp"
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
                std::unordered_map<uint32_t, bool>& selectedObjects,
                VkPipelineRenderingCreateInfo attachmentInfo);

  ~OutlineSystem();

  OutlineSystem(const OutlineSystem&) = delete;
  OutlineSystem& operator=(const OutlineSystem&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  void createSamplers();

  VkDescriptorSet& textureDescriptorSet;
  std::vector<EngineTexture> shaderSamplerImages;
  std::unordered_map<uint32_t, bool>& selectedObjects;
};

}  // namespace GameEngine
