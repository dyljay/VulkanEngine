#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_renderer.hpp"
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
                EngineDescriptorSetLayout& descriptorSetLayouts,
                std::unordered_map<uint32_t, bool>& selectedObjects,
                VkPipelineRenderingCreateInfo pipelineRenderingInfo,
                OffScreenRenderer& offscreenRenderer,
                EngineDescriptorPoolGrowable& globalPool);

  ~OutlineSystem();

  OutlineSystem(const OutlineSystem&) = delete;
  OutlineSystem& operator=(const OutlineSystem&) = delete;

  void render(FrameInfo& frameinfo) override;

 private:
  void createTextures(EngineDevice& device,
                      OffScreenRenderer& offscreenRenderer);

  void createDescriptorSet(EngineDevice& device,
                           EngineDescriptorSetLayout& setLayout,
                           EngineDescriptorPoolGrowable& growablePool);

  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<std::unique_ptr<EngineTexture>> shaderSamplerImages;
  std::unordered_map<uint32_t, bool>& selectedObjects;
};

}  // namespace GameEngine
