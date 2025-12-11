#pragma once

#include "engine_device.hpp"

#include <vector>
#include <string>

namespace GameEngine {

struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;
    
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class GraphicsPipeline {

public:
    GraphicsPipeline(EngineDevice& device,
                     const std::string& vertPath,
                     const std::string& fragPath,
                     const PipelineConfigInfo& configInfo);
    
    ~GraphicsPipeline();
    
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline &operator=(const GraphicsPipeline&) = delete;
    
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
    
    void bind(VkCommandBuffer commandBuffer);
    
private:
    void createGraphicsPipeline(const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);
    
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
    
    static std::vector<char> readFile(const std::string& fileName);
    
    EngineDevice &geDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};

}
