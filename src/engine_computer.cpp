#include "engine_computer.hpp"

#include <cstddef>
#include <cstdint>
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
  for (int i = 0; i < numProcesses; i++) {
    vkDestroyFence(geDevice.device(), commandBufferAvaialable[i], nullptr);
  }

  vkFreeCommandBuffers(geDevice.device(),
                       geDevice.getCommandPool(),
                       numProcesses,
                       commandBuffers.data());
}

void EngineComputer::createSyncObjects()
{
  commandBufferAvaialable.resize(numProcesses, VK_NULL_HANDLE);

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < numProcesses; i++) {
    if (vkCreateFence(geDevice.device(),
                      &fenceInfo,
                      nullptr,
                      &commandBufferAvaialable[i]))
    {
      throw std::runtime_error("failed to create compute pipeline fence");
    }
  }
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

VkCommandBuffer EngineComputer::getAvailableCommandBuffer()
{
  VkCommandBuffer commandBuffer;

  return commandBuffer;
}

VkCommandBuffer EngineComputer::beginComputation(int i)
{
  VkCommandBuffer commandBuffer = commandBuffers[i];
  index = i;
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (commandBufferAvaialable[index] != VK_NULL_HANDLE) {
    vkWaitForFences(geDevice.device(),
                    1,
                    &commandBufferAvaialable[index],
                    VK_TRUE,
                    UINT16_MAX);
  }

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

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkResetFences(geDevice.device(), 1, &commandBufferAvaialable[index]);
  if (vkQueueSubmit(geDevice.computeQueue(),
                    1,
                    &submitInfo,
                    commandBufferAvaialable[index]) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit compute command buffer");
  }
}
}  // namespace GameEngine
