#include "engine_image.hpp"
#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace GameEngine {

EngineImage::EngineImage(EngineDevice &geDevice, ImageConfigInfo imageInfo,
                         ImageViewConfigInfo imageViewConfigInfo)
    : geDevice{geDevice}, imageExtent(imageInfo.extent),
      layerCount(imageViewConfigInfo.layerCount), format(imageInfo.format),
      imageType(imageInfo.imageType) {

  createImage(imageInfo);
  createImageView(imageViewConfigInfo);
}

EngineImage::~EngineImage() {
  vkDestroyImageView(geDevice.device(), imageView, nullptr);

  vmaDestroyImage(geDevice.getAllocator(), image, imageAllocation);
}

void EngineImage::createImage(ImageConfigInfo imageConfigInfo) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = imageConfigInfo.imageType;
  imageInfo.extent.width = imageExtent.width;
  imageInfo.extent.height = imageExtent.height;
  imageInfo.extent.depth = imageExtent.depth;
  imageInfo.mipLevels = imageConfigInfo.mipLevels;
  imageInfo.arrayLayers = imageConfigInfo.arrayLayers;
  imageInfo.format = format;
  imageInfo.tiling = imageConfigInfo.tiling;
  imageInfo.usage = imageConfigInfo.usage;
  imageInfo.sharingMode = imageConfigInfo.sharingMode;
  imageInfo.samples = imageConfigInfo.samples;
  imageInfo.flags = imageConfigInfo.flags;

  geDevice.createImageWithInfo(imageInfo, imageConfigInfo.memPropertyFlags,
                               image, imageAllocation);
}

void EngineImage::createImageView(ImageViewConfigInfo imageViewConfigInfo) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = imageViewConfigInfo.viewType;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = imageViewConfigInfo.aspectMask;
  viewInfo.subresourceRange.baseArrayLayer = imageViewConfigInfo.baseArrayLayer;
  viewInfo.subresourceRange.baseMipLevel = imageViewConfigInfo.baseMipLevel;
  viewInfo.subresourceRange.layerCount = layerCount;
  viewInfo.subresourceRange.levelCount = imageViewConfigInfo.levelCount;
  viewInfo.pNext = imageViewConfigInfo.pNext;

  if (vkCreateImageView(geDevice.device(), &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create image view");
  }
}

void EngineImage::transitionImageLayout(VkImageLayout oldLayout,
                                        VkImageLayout newLayout) {}

void EngineImage::setImageBarrierToPipeline() {
  if (auto commandBuffer = geDevice.beginSingleTimeCommands()) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    geDevice.endSingleTimeCommands(commandBuffer);
  }
}

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
