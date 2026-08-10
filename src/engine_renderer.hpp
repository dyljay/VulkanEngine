#pragma once

#include "engine_buffer.hpp"
#include "engine_device.hpp"
#include "engine_image.hpp"
#include "engine_swapchain.hpp"
#include "engine_window.hpp"
#include "vulkan/vulkan_core.h"
// std
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace GameEngine {
class EngineRenderer {
 public:
  EngineRenderer(EngineWindow& window, EngineDevice& device);
  ~EngineRenderer();

  EngineDevice& getDevice() const { return geDevice; }
  EngineWindow& getWindow() const { return geWindow; }

  EngineRenderer(const EngineRenderer&) = delete;
  EngineRenderer& operator=(const EngineRenderer&) = delete;

  float getAspectRatio() const { return geSwapChain->extentAspectRatio(); }
  bool isFrameInProgress() const { return isFrameStarted; }

  VkCommandBuffer getCurrentCommandBuffer() const
  {
    assert(isFrameStarted &&
           "Cannot get command buffer when frame is not in "
           "progress");
    return commandBuffers[currentFrameIndex];
  }

  int getFrameIndex() const
  {
    assert(isFrameStarted &&
           "Cannot get frame index when frame is not in progress");
    return currentFrameIndex;
  }

  VkPipelineRenderingCreateInfo getPipelineRenderingInfo() const
  {
    return pipelineCreate;
  };

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginRender(VkCommandBuffer commandBuffer);
  void endRender(VkCommandBuffer commandBuffer);

 private:
  void createCommandBuffers();
  void createPipelineCreateInfo();
  void freeCommandBuffers();
  void recreateSwapChain();

  EngineWindow& geWindow;
  EngineDevice& geDevice;
  std::unique_ptr<EngineSwapChain> geSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;
  VkPipelineRenderingCreateInfo pipelineCreate{};
  std::vector<VkFormat> colorAttachmentFormats;

  uint32_t currentImageIndex;
  int currentFrameIndex = 0;
  bool isFrameStarted = false;
};

class OffScreenRenderer {
 public:
  OffScreenRenderer(EngineWindow& geWindow, EngineDevice& geDevice);
  ~OffScreenRenderer();

  EngineDevice& getDevice() const { return geDevice; }
  EngineWindow& getWindow() const { return geWindow; }

  OffScreenRenderer(const OffScreenRenderer&) = delete;
  OffScreenRenderer& operator=(const OffScreenRenderer&) = delete;

  std::unique_ptr<EngineImage>& getImage() { return offscreenImage; }

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginRenderPass(VkCommandBuffer commandBuffer);
  void endRenderPass(VkCommandBuffer commandBuffer);

  template <typename T>
  T* getPixelData(uint32_t x, uint32_t y)
  {
    assert(offscreenImage->getTiling() == VK_IMAGE_TILING_LINEAR &&
           "Tiling must be linear to read directly from image");

    vkWaitForFences(geDevice.device(),
                    1,
                    &imageRenderedFence,
                    VK_TRUE,
                    UINT64_MAX);

    void* mapped = nullptr;
    vmaMapMemory(geDevice.getAllocator(),
                 offscreenImage->getAllocation(),
                 &mapped);

    T* data_t = static_cast<T*>(mapped);
    T* dataPoint = &data_t[y * imageExtent.width + x];

    vmaUnmapMemory(geDevice.getAllocator(), offscreenImage->getAllocation());

    return dataPoint;
  }

  VkResult submitCommandBuffers(const VkCommandBuffer* buffers,
                                uint32_t* imageIndex);

  VkResult copyImageToBuffer();

 private:
  void init();

  void createOffscreenImage();
  void createDepthResources();
  void createSyncObject();
  void createCommandBuffer();

  void freeResources();
  void freeCommandBuffer();

  VkResult submitCommandBuffer();

  EngineDevice& geDevice;
  EngineWindow& geWindow;

  VkRenderPass renderPass;
  VkCommandBuffer commandBuffer;

  std::unique_ptr<EngineImage> offscreenImage;
  std::unique_ptr<EngineImage> depthImage;

  VkExtent2D imageExtent;

  VkFramebuffer frameBuffer;

  VkFence imageRenderedFence;

  bool isFrameStarted = false;
};
}  // namespace GameEngine
