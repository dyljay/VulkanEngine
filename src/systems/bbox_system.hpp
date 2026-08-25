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
               const Shader& shaders,
               const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
               const std::vector<VkDescriptorSet>& uboSets,
               VkPipelineRenderingCreateInfo attachmentInfo);

  ~BboxRenderer();

  BboxRenderer(const BboxRenderer&) = delete;
  BboxRenderer& operator=(const BboxRenderer&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  const std::vector<VkDescriptorSet>& uboSets;
};
}  // namespace GameEngine
