#include "engine_pipeline.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "engine_device.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

GraphicsPipeline::GraphicsPipeline(EngineDevice& device,
                                   const std::string& vertPath,
                                   const std::string& fragPath,
                                   const PipelineConfigInfo& configInfo)
    : geDevice{device}
{
  createGraphicsPipeline(vertPath, fragPath, configInfo);
}

GraphicsPipeline::~GraphicsPipeline()
{
  vkDestroyShaderModule(geDevice.device(), vertShaderModule, nullptr);
  vkDestroyShaderModule(geDevice.device(), fragShaderModule, nullptr);

  vkDestroyPipeline(geDevice.device(), graphicsPipeline, nullptr);
}

void GraphicsPipeline::createGraphicsPipeline(
    const std::string& vertPath,
    const std::string& fragPath,
    const PipelineConfigInfo& configInfo)
{
  assert(configInfo.pipelineLayout != VK_NULL_HANDLE &&
         "Cannot create graphics pipeline:: no pipelineLayout provided in "
         "configInfo");

  assert(configInfo.renderingAttachmentInfo != VK_NULL_HANDLE &&
         "Cannot create graphics pipeline:: no attachment descriptions "
         "provided in configInfo");

  auto vertShaderCode = readFile(vertPath);
  auto fragShaderCode = readFile(fragPath);

  createShaderModule(geDevice, vertShaderCode, &vertShaderModule);
  createShaderModule(geDevice, fragShaderCode, &fragShaderModule);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";
  vertShaderStageInfo.flags = 0;
  vertShaderStageInfo.pNext = nullptr;
  vertShaderStageInfo.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";
  fragShaderStageInfo.flags = 0;
  fragShaderStageInfo.pNext = nullptr;
  fragShaderStageInfo.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  auto bindingDescriptions = configInfo.bindingDescriptions;
  auto attributeDescriptions = configInfo.attributeDescriptions;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
  pipelineInfo.pViewportState = &configInfo.viewportInfo;
  pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
  pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
  pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
  pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
  pipelineInfo.layout = configInfo.pipelineLayout;
  pipelineInfo.subpass = configInfo.subpass;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.pNext = configInfo.renderingAttachmentInfo;

  if (vkCreateGraphicsPipelines(geDevice.device(),
                                VK_NULL_HANDLE,
                                1,
                                &pipelineInfo,
                                nullptr,
                                &graphicsPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }
}

void GraphicsPipeline::bind(VkCommandBuffer commandBuffer)
{
  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);
}

void GraphicsPipeline::createShaderModule(EngineDevice& geDevice,
                                          const std::vector<char>& code,
                                          VkShaderModule* shaderModule)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  if (vkCreateShaderModule(geDevice.device(),
                           &createInfo,
                           nullptr,
                           shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }
}

void GraphicsPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo)
{
  configInfo.inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  configInfo.viewportInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  configInfo.viewportInfo.viewportCount = 1;
  configInfo.viewportInfo.pViewports = nullptr;
  configInfo.viewportInfo.scissorCount = 1;
  configInfo.viewportInfo.pScissors = nullptr;

  configInfo.rasterizationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
  configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  configInfo.rasterizationInfo.lineWidth = 1.0f;
  configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
  configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
  configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
  configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

  configInfo.multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
  configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
  configInfo.multisampleInfo.minSampleShading = 1.0f;
  configInfo.multisampleInfo.pSampleMask = nullptr;
  configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
  configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

  configInfo.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
  configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  configInfo.colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
  configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  configInfo.colorBlendInfo.attachmentCount = 1;
  configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
  configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
  configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
  configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
  configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

  configInfo.depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  configInfo.depthStencilInfo.minDepthBounds = 0.0f;
  configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
  configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
  configInfo.depthStencilInfo.front = {};
  configInfo.depthStencilInfo.back = {};

  configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  configInfo.dynamicStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  configInfo.dynamicStateInfo.pDynamicStates =
      configInfo.dynamicStateEnables.data();
  configInfo.dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
  configInfo.dynamicStateInfo.flags = 0;
}

void GraphicsPipeline::enableAlphaBlending(PipelineConfigInfo& configInfo)
{
  configInfo.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
  configInfo.colorBlendAttachment.srcColorBlendFactor =
      VK_BLEND_FACTOR_SRC_ALPHA;
  configInfo.colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

std::vector<char> GraphicsPipeline::readFile(const std::string& fileName)
{
  std::ifstream file(fileName, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file: " + fileName);
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

/////////////////////////////////
// ---- Compute Pipeline ----- //
/////////////////////////////////

ComputePipeline::ComputePipeline(EngineDevice& geDevice,
                                 const std::string& shader,
                                 VkDescriptorSetLayout descriptorSetLayout,
                                 uint32_t pushConstantSize)
    : geDevice{geDevice}
{
  createPipelineLayout(descriptorSetLayout, pushConstantSize);
  createComputePipeline(shader);
}

ComputePipeline::~ComputePipeline()
{
  vkDestroyShaderModule(geDevice.device(), compShaderModule, nullptr);
  vkDestroyPipelineLayout(geDevice.device(), computePipelineLayout, nullptr);
  vkDestroyPipeline(geDevice.device(), computePipeline, nullptr);
}

void ComputePipeline::createPipelineLayout(
    VkDescriptorSetLayout descriptorSetLayout,
    uint32_t pushConstantSize)
{
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = pushConstantSize;

  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(geDevice.device(),
                             &pipelineLayoutInfo,
                             nullptr,
                             &computePipelineLayout))
  {
    throw std::runtime_error("failed to create compute pipeline layout");
  }
}

void ComputePipeline::createComputePipeline(const std::string& shader)
{
  auto compShaderCode = GraphicsPipeline::readFile(shader);

  GraphicsPipeline::createShaderModule(geDevice,
                                       compShaderCode,
                                       &compShaderModule);

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = compShaderModule;
  computeShaderStageInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = computeShaderStageInfo;
  pipelineInfo.layout = computePipelineLayout;

  if (vkCreateComputePipelines(geDevice.device(),
                               VK_NULL_HANDLE,
                               1,
                               &pipelineInfo,
                               nullptr,
                               &computePipeline))
  {
    throw std::runtime_error("failed to create compute pipeline");
  }
}

void ComputePipeline::bind(VkCommandBuffer commandBuffer)
{
  vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    computePipeline);
}

void ComputePipeline::dispatch(VkCommandBuffer commandBuffer,
                               const VkDescriptorSet* descriptorSets,
                               uint32_t n)
{
  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          computePipelineLayout,
                          0,
                          1,
                          descriptorSets,
                          0,
                          nullptr);

  vkCmdDispatch(commandBuffer, groupCount(n, 256), 1, 1);
}

}  // namespace GameEngine
