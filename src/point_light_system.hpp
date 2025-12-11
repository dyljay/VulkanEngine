#pragma once

#include "engine_pipeline.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_game_object.hpp"
#include "engine_camera.hpp"

// std
#include <memory>
#include <vector>

namespace GameEngine {
class PointLightSystem {
public:
    PointLightSystem(EngineDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();
    
    PointLightSystem(const PointLightSystem&) = delete;
    PointLightSystem &operator=(const PointLightSystem&) = delete;
    
    void update(FrameInfo &frameinfo, GlobalUbo &ubo);
    
    void render(FrameInfo &frameinfo);
    
private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);
    
    EngineDevice &geDevice;
    std::unique_ptr<GraphicsPipeline> gePipeline;
    VkPipelineLayout pipelineLayout;
};
}
