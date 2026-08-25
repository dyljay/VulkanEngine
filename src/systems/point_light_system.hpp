#pragma once

#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "system.hpp"

// std
#include <cstdint>
#include <vector>

namespace GameEngine {
class PointLightSystem : RenderSystem {
 public:
  PointLightSystem(
      EngineDevice& device,
      const Shader& shaders,
      const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
      const std::vector<VkDescriptorSet>& uboSets,
      VkPipelineRenderingCreateInfo attachmentInfo);

  ~PointLightSystem();

  PointLightSystem(const PointLightSystem&) = delete;
  PointLightSystem& operator=(const PointLightSystem&) = delete;

  void update(FrameInfo& frameinfo, GlobalUbo& ubo);

  void render(FrameInfo& frameinfo) override;

 private:
  const std::vector<VkDescriptorSet>& uboSets;
};
}  // namespace GameEngine
