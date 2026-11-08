#pragma once

#include "engine_device.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "vulkan/vulkan_core.h"

// vulkan headers
#include <sys/types.h>
#include <vulkan/vulkan.h>

// std lib headers
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {

class EngineSwapChain {
 public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

  EngineSwapChain(EngineDevice& deviceRef, VkExtent2D windowExtent);
  EngineSwapChain(EngineDevice& deviceRef,
                  VkExtent2D windowExtent,
                  std::shared_ptr<EngineSwapChain> previous);
  ~EngineSwapChain();

  EngineSwapChain(const EngineSwapChain&) = delete;
  EngineSwapChain& operator=(const EngineSwapChain&) = delete;

  VkImageView getImageView(int index) { return swapChainImageViews[index]; }
  VkImageView getDepthView(int index) { return depthImageViews[index]; }
  VkImageView getResourceView(int index) { return multiSampleViews[index]; }

  size_t imageCount() { return swapChainImages.size(); }
  VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
  VkFormat getDepthImageFormat() { return swapChainDepthFormat; }
  VkExtent2D getSwapChainExtent() { return swapChainExtent; }
  uint32_t width() { return swapChainExtent.width; }
  uint32_t height() { return swapChainExtent.height; }

  float extentAspectRatio()
  {
    return static_cast<float>(swapChainExtent.width) /
           static_cast<float>(swapChainExtent.height);
  }
  VkFormat findDepthFormat();

  VkResult acquireNextImage(uint32_t* imageIndex);
  VkResult submitCommandBuffers(const VkCommandBuffer* buffers,
                                uint32_t* imageIndex);

  void transitionImageLayouts(VkCommandBuffer commandBuffer,
                              uint32_t currentIndex);
  void transitionPresentImageLayout(VkCommandBuffer commandBuffer,
                                    uint32_t currentIndex);

  bool compareSwapFormats(const EngineSwapChain& swapChain) const
  {
    return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
           swapChain.swapChainImageFormat == swapChainImageFormat;
  }

 private:
  void init();
  void createSwapChain();
  void createImageViews();
  void createColorResources();
  void createDepthResources();
  void createSyncObjects();

  // Helper functions
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  VkFormat swapChainImageFormat;
  VkFormat swapChainDepthFormat;
  VkExtent2D swapChainExtent;
  VkSampleCountFlagBits swapChainSampleCount;

  std::vector<VkImage> depthImages;
  std::vector<VmaAllocation> depthImageAlloc;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkImage> multiSampleImages;
  std::vector<VmaAllocation> multiSampleAlloc;
  std::vector<VkImageView> multiSampleViews;

  EngineDevice& device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  std::shared_ptr<EngineSwapChain> oldSwapChain;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> imagesInFlight;
  size_t currentFrame = 0;
};

}  // namespace GameEngine
