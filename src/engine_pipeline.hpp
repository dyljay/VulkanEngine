#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"

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
  VkPipelineRenderingCreateInfo* renderingAttachmentInfo = nullptr;
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
  GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

  static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
  static void enableAlphaBlending(PipelineConfigInfo& configInfo);

  static std::vector<char> readFile(const std::string& fileName);

  static void createShaderModule(EngineDevice& geDevice,
                                 const std::vector<char>& code,
                                 VkShaderModule* shaderModule);

  void bind(VkCommandBuffer commandBuffer);

 private:
  void createGraphicsPipeline(const std::string& vertPath,
                              const std::string& fragPath,
                              const PipelineConfigInfo& configInfo);

  EngineDevice& geDevice;
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
};

class ComputePipeline {
 public:
  ComputePipeline(EngineDevice& geDevice,
                  const std::string& shader,
                  VkDescriptorSetLayout descriptorSetLayout,
                  uint32_t pushConstantSize);

  ~ComputePipeline();

  ComputePipeline(const ComputePipeline&) = delete;
  ComputePipeline& operator=(const ComputePipeline&) = delete;

  void bind(VkCommandBuffer commadBuffer);

  void dispatch(VkCommandBuffer commandBuffer,
                const std::vector<VkDescriptorSet>& descroptorSets);

 private:
  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout,
                            uint32_t pushConstantSize);

  void createComputePipeline(const std::string& shader);

  EngineDevice& geDevice;
  VkShaderModule compShaderModule;
  VkPipeline computePipeline;
  VkPipelineLayout computePipelineLayout;
};

}  // namespace GameEngine
