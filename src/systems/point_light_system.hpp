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
        Shader shaders,
        const std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
        VkRenderPass renderPass);

    ~PointLightSystem();

    PointLightSystem(const PointLightSystem&) = delete;
    PointLightSystem& operator=(const PointLightSystem&) = delete;

    void update(FrameInfo& frameinfo, GlobalUbo& ubo);

    void render(FrameInfo& frameinfo, DescriptorSets& descriptorSets);
};
}  // namespace GameEngine
