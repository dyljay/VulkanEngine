#include "engine_computer.hpp"

#include <stdexcept>

#include "vulkan/vulkan_core.h"

namespace GameEngine {

EngineComputer::EngineComputer(EngineDevice& geDevice, int processes)
    : geDevice{geDevice},
      numProcesses{processes}
{
  createCommandBuffers();
  createSyncObjects();
}

EngineComputer::~EngineComputer()
{
  vkFreeCommandBuffers(geDevice.device(),
                       geDevice.getCommandPool(),
                       numProcesses,
                       commandBuffers.data());
}

void EngineComputer::createCommandBuffers()
{
  commandBuffers.resize(numProcesses);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = geDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(geDevice.device(),
                               &allocInfo,
                               commandBuffers.data()))
  {
    throw std::runtime_error(
        "failed to allocate command buffer for compute pipelines");
  }
}

VkCommandBuffer EngineComputer::getAvailableCommandBuffer() {}

VkCommandBuffer EngineComputer::beginComputation()
{
  VkCommandBuffer commandBuffer = getAvailableCommandBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer");
  }

  return commandBuffer;
}

void EngineComputer::endComputation(VkCommandBuffer commandBuffer)
{
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to end computation command buffer");
  }
}
}  // namespace GameEngine
