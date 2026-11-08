#pragma once

#include <vector>

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"
namespace GameEngine {

class EngineComputer {
 public:
  EngineComputer(EngineDevice& device, int processes);
  ~EngineComputer();

  VkCommandBuffer beginComputation(int i);
  void endComputation(VkCommandBuffer commandBuffer);

 private:
  void createCommandBuffers();
  void createSyncObjects();

  VkCommandBuffer getAvailableCommandBuffer();

  EngineDevice& geDevice;
  std::vector<VkCommandBuffer> commandBuffers;
  int index = 0;

  std::vector<VkFence> commandBufferAvaialable;
  int numProcesses;
};
}  // namespace GameEngine
