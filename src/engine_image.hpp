#pragma once

#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

namespace GameEngine {

class EngineImage {
public:
  EngineImage(EngineDevice &geDevice, VkExtent3D imageExtent,
              VkImageUsageFlags imageUseFlags,
              VkMemoryPropertyFlags memoryFlags, VkImageViewType viewType,
              VkImageUsageFlags imageViewUseFlags, VkImageCreateFlags flags = 0,
              VkImageType imageType = VK_IMAGE_TYPE_2D,
              VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
              VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL,
              uint32_t mipLevels = 1, uint32_t arrayLayers = 1,
              VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
              VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
              VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
              VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0);

  ~EngineImage();

  void createImage(VkExtent3D imageExtent, VkImageUsageFlags imageUseFlags,
                   VkImageCreateFlags createImageFlags, VkImageType imageType,
                   VkFormat imageFormat, VkImageTiling imageTiling,
                   uint32_t mipLevels, uint32_t arrayLayers,
                   VkSampleCountFlagBits samples, VkSharingMode sharingMode,
                   VkImageLayout initialLayout,
                   VkMemoryPropertyFlags memoryFlags);

  void createImageView(VkImageViewType viewType,
                       VkImageUsageFlags imageViewUseFlags, VkFormat format,
                       VkImageAspectFlags aspectMask, uint32_t baseMipLevel,
                       uint32_t baseArrayLayer);

  void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
  void setImageBarrierToPipeline();

private:
  VkExtent3D imageExtent;
  uint32_t layerCount;
  VkFormat format;
  VkImageType imageType;

  VkImage image;

  // TODO: double check how allocation is handled here to see if you should work
  // with a pointer here
  VmaAllocation imageAllocation;

  VkImageView imageView;

  EngineDevice &geDevice;
};
} // namespace GameEngine
