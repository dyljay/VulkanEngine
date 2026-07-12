#include "engine_image.hpp"
#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <stdexcept>

namespace GameEngine {

/**
 * Initializes image object with VkImage and VkImageView
 *
 * @param geDevice The logical device in use
 * @param imageExtent The bounds of the image -> type 3D so it includes width,
 * height, and depth
 * @param imageUseFlags
 * @param memoryFlags
 * @param viewType
 * @param imageViewUseFlags
 * @param flags
 * @param imageType
 * @param format
 * @param imageTiling
 * @param mipLevels
 * @param arrayLayers
 * @param samples
 * @param sharingMode
 * @param initialLayout
 * @param aspectMask
 * @param baseMipLevel
 * @param baseArrayLayer
 */

EngineImage::EngineImage(
    EngineDevice &geDevice, VkExtent3D imageExtent,
    VkImageUsageFlags imageUseFlags, VkMemoryPropertyFlags memoryFlags,
    VkImageViewType viewType, VkImageUsageFlags imageViewUseFlags,
    VkImageCreateFlags flags, VkImageType imageType, VkFormat format,
    VkImageTiling imageTiling, uint32_t mipLevels, uint32_t arrayLayers,
    VkSampleCountFlagBits samples, VkSharingMode sharingMode,
    VkImageLayout initialLayout, VkImageAspectFlags aspectMask,
    uint32_t baseMipLevel, uint32_t baseArrayLayer)
    : geDevice{geDevice}, imageExtent{imageExtent}, layerCount{arrayLayers},
      imageType{imageType} {

  createImage(imageExtent, imageUseFlags, flags, imageType, format, imageTiling,
              mipLevels, arrayLayers, samples, sharingMode, initialLayout,
              memoryFlags);

  createImageView(viewType, imageViewUseFlags, format, aspectMask, baseMipLevel,
                  baseArrayLayer);
}

EngineImage::~EngineImage() {
  vkDestroyImageView(geDevice.device(), imageView, nullptr);

  vmaDestroyImage(geDevice.getAllocator(), image, imageAllocation);
}

void EngineImage::createImage(
    VkExtent3D imageExtent, VkImageUsageFlags imageUseFlags,
    VkImageCreateFlags flags, VkImageType imageType, VkFormat format,
    VkImageTiling imageTiling, uint32_t mipLevels, uint32_t arrayLayers,
    VkSampleCountFlagBits samples, VkSharingMode sharingMode,
    VkImageLayout initialLayout, VkMemoryPropertyFlags memoryFlags) {

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = imageType;
  imageInfo.extent.width = imageExtent.width;
  imageInfo.extent.height = imageExtent.height;
  imageInfo.extent.depth = imageExtent.depth;
  imageInfo.mipLevels = mipLevels;
  imageInfo.arrayLayers = arrayLayers;
  imageInfo.format = format;
  imageInfo.tiling = imageTiling;
  imageInfo.usage = imageUseFlags;
  imageInfo.sharingMode = sharingMode;
  imageInfo.samples = samples;
  imageInfo.flags = flags;

  geDevice.createImageWithInfo(imageInfo, memoryFlags, image, imageAllocation);
}

void EngineImage::createImageView(VkImageViewType viewType,
                                  VkImageUsageFlags imageViewUseFlags,
                                  VkFormat format,
                                  VkImageAspectFlags aspectMask,
                                  uint32_t baseMipLevel,
                                  uint32_t baseArrayLayer) {

  VkImageViewUsageCreateInfo usageInfo{};
  usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
  usageInfo.usage = imageViewUseFlags;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = viewType;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectMask;
  viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
  viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
  viewInfo.subresourceRange.layerCount = layerCount;
  viewInfo.pNext = &usageInfo;

  if (vkCreateImageView(geDevice.device(), &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view");
  }
}

void EngineImage::transitionImageLayout(VkImageLayout oldLayout,
                                        VkImageLayout newLayout) {}

void EngineImage::setImageBarrierToPipeline() {}

/**
 * //before//
 * oldLayout - VK_IMAGE_LAYOUT_UNDEFINED
 * srcAccessMask - 0
 * srcStage -
 * newLayout - VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
 * dstAccessMask - VK_ACCESS_TRANSFER_READ_BIT
 * dstStage -
 *
 * //after//
 * oldLayout - VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
 * newLayout -
 */
} // namespace GameEngine
