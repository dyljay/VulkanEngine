#pragma once

#include "engine_device.hpp"
#include "engine_swapchain.hpp"
#include "engine_window.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace GameEngine {
class EngineRenderer {
public:
  EngineRenderer(EngineWindow &window, EngineDevice &device);
  ~EngineRenderer();

  EngineDevice &getDevice() const { return geDevice; }
  EngineWindow &getWindow() const { return geWindow; }

  EngineRenderer(const EngineRenderer &) = delete;
  EngineRenderer &operator=(const EngineRenderer &) = delete;

  VkRenderPass getSwapChainRenderPass() const {
    return geSwapChain->getRenderPass();
  }
  float getAspectRatio() const { return geSwapChain->extentAspectRatio(); }
  bool isFrameInProgress() const { return isFrameStarted; }

  VkCommandBuffer getCurrentCommandBuffer() const {
    assert(isFrameStarted &&
           "Cannot get command buffer when frame is not in progress");
    return commandBuffers[currentFrameIndex];
  }

  int getFrameIndex() const {
    assert(isFrameStarted &&
           "Cannot get frame index when frame is not in progress");
    return currentFrameIndex;
  }

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapChain();

  EngineWindow &geWindow;
  EngineDevice &geDevice;
  std::unique_ptr<EngineSwapChain> geSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t currentImageIndex;
  int currentFrameIndex = 0;
  bool isFrameStarted = false;
};
} // namespace GameEngine
