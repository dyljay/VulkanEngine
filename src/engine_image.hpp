#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

struct ImageConfigInfo {
    VkImageCreateFlags flags = 0;
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D extent = {0, 0, 1};
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkMemoryPropertyFlags memPropertyFlags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
};

struct ImageViewConfigInfo {
    VkImageViewCreateFlags flags = 0;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t baseMipLevel = 0;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
    uint32_t levelCount = 1;
    VkImageUsageFlags viewUsage;
    const void* pNext = nullptr;
};

class EngineImage {
   public:
    EngineImage(EngineDevice& geDevice,
                ImageConfigInfo imageInfo,
                ImageViewConfigInfo imageViewInfo);

    ~EngineImage();

    VkImage getImage() const { return image; }
    VkImageView getImageView() const { return imageView; }
    VkFormat getImageFormat() const { return format; }

    VmaAllocation getAllocation() const { return imageAllocation; }
    VkImageTiling getTiling() const { return tiling; }

    VkExtent3D getExtent() const { return imageExtent; }
    uint32_t getWidth() const { return imageExtent.width; }
    uint32_t getHeight() const { return imageExtent.height; }
    uint32_t getLayerCount() const { return layerCount; }

    float extentAspectRatio()
    {
        return static_cast<float>(imageExtent.width) /
               static_cast<float>(imageExtent.height);
    }

    void imageMemoryBarrier(VkCommandBuffer commandBuffer,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout);

   private:
    void createImage(ImageConfigInfo imageConfigInfo);

    void createImageView(ImageViewConfigInfo imageViewConfigInfo);

    bool hasStencilComponent(VkFormat format);

    VkExtent3D imageExtent;
    uint32_t layerCount;
    VkFormat format;
    VkImageType imageType;
    VkImage image;
    VkImageTiling tiling;
    VkImageAspectFlags aspect;
    uint32_t baseMipLevel;
    uint32_t baseArrayLayer;
    uint32_t levelCount;

    VmaAllocation imageAllocation;

    VkImageView imageView;

    EngineDevice& geDevice;
};
}  // namespace GameEngine
