#pragma once

#include "engine_buffer.hpp"
#include "engine_device.hpp"
#include "engine_swapchain.hpp"
#include "engine_window.hpp"
#include "vulkan/vulkan_core.h"

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

class OffScreenRenderer {
public:
  OffScreenRenderer(EngineWindow &geWindow, EngineDevice &geDevice);
  ~OffScreenRenderer();

  EngineDevice &getDevice() const { return geDevice; }
  EngineWindow &getWindow() const { return geWindow; }

  OffScreenRenderer(const OffScreenRenderer &) = delete;
  OffScreenRenderer &operator=(const OffScreenRenderer &) = delete;

  VkRenderPass getRenderPass() const { return renderPass; }

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginRenderPass(VkCommandBuffer commandBuffer);
  void endRenderPass(VkCommandBuffer commandBuffer);
  VkFormat getImageFormat() { return imageFormat; }
  VkExtent2D getExtent() { return imageExtent; }
  uint32_t width() { return imageExtent.width; }
  uint32_t height() { return imageExtent.height; }

  float extentAspectRatio() {
    return static_cast<float>(imageExtent.width) /
           static_cast<float>(imageExtent.height);
  }
  VkFormat findDepthFormat();

  VkImageView getImageView() const { return offScreenImageView; }

  VkResult submitCommandBuffers(const VkCommandBuffer *buffers,
                                uint32_t *imageIndex);

private:
  void init();

  void createImage();
  void createImageView();
  void createDepthResource();
  void createFramebuffer();
  void createRenderPass();
  void createSyncObject();
  void createCommandBuffer();

  void freeResources();
  void freeCommandBuffer();

  VkResult submitCommandBuffer();

  EngineDevice &geDevice;
  EngineWindow &geWindow;

  VkRenderPass renderPass;
  VkCommandBuffer commandBuffer;

  VkImage offscreenImage;
  VkImageView offScreenImageView;
  VkImage depthImage;
  VkImageView depthImageView;

  VkFormat imageFormat;
  VkFormat depthFormat;

  // TODO: do i need to have two allocators?
  VmaAllocation depthAllocation;
  VmaAllocation offscreenAllocation;

  VkFramebuffer frameBuffer;
  VkExtent2D imageExtent;

  VkSemaphore imageAvailableSemaphore;
  VkFence imageRenderedFence;

  bool isFrameStarted = false;

  std::unique_ptr<EngineBuffer> readBuffer;
};
} // namespace GameEngine
