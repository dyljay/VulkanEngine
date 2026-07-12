#pragma once

#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>

namespace GameEngine {

struct ImageConfigInfo {
  VkImageCreateFlags flags;
  VkImageType imageType = VK_IMAGE_TYPE_2D;
  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
  VkExtent2D extent;
  uint32_t mipLevels = 0;
  uint32_t arrayLayers = 1;
  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkImageUsageFlags usage;
  VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkMemoryPropertyFlags memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
};

struct ImageViewConfigInfo {
  VkImageViewCreateFlags flags;
  VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  uint32_t baseMipLevel = 0;
  uint32_t baseArrayLayer = 0;
  uint32_t layerCount = 1;
  VkImageUsageFlags viewUsage;
};

class EngineImage {
public:
  EngineImage(EngineDevice &geDevice, ImageConfigInfo imageInfo,
              ImageViewConfigInfo imageViewInfo);

  ~EngineImage();

  VkImage getImage() const { return image; }
  VkImageView getImageView() const { return imageView; }
  VkFormat getImageFormat() const { return format; }

  VkExtent2D getExtent() const { return imageExtent; }
  uint32_t getWidth() const { return imageExtent.width; }
  uint32_t getHeight() const { return imageExtent.height; }
  uint32_t getLayerCount() const { return layerCount; }

  float extentAspectRatio() {
    return static_cast<float>(imageExtent.width) /
           static_cast<float>(imageExtent.height);
  }

private:
  void verifyParameters(ImageConfigInfo imageInfo,
                        ImageViewConfigInfo imageViewInfo);

  void createImage(ImageConfigInfo imageConfigInfo);

  void createImageView(ImageViewConfigInfo imageViewConfigInfo);

  void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

  void setImageBarrierToPipeline();

  VkExtent2D imageExtent;
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
