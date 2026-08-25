#include "engine_image.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {
EngineImage::EngineImage(EngineDevice& geDevice,
                         ImageConfigInfo imageInfo,
                         ImageViewConfigInfo imageViewConfigInfo)
    : geDevice{geDevice},
      imageExtent(imageInfo.extent),
      layerCount(imageViewConfigInfo.layerCount),
      format(imageInfo.format),
      imageType(imageInfo.imageType),
      tiling{imageInfo.tiling},
      aspect{imageViewConfigInfo.aspectMask},
      baseMipLevel{imageViewConfigInfo.baseMipLevel},
      baseArrayLayer{imageViewConfigInfo.baseArrayLayer},
      levelCount{imageViewConfigInfo.levelCount}
{
  createImage(imageInfo);
  createImageView(imageViewConfigInfo);
}

EngineImage::~EngineImage()
{
  vkDestroyImageView(geDevice.device(), imageView, nullptr);

  vmaDestroyImage(geDevice.getAllocator(), image, imageAllocation);
}

void EngineImage::createImage(ImageConfigInfo imageConfigInfo)
{
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

  geDevice.createImageWithInfo(imageInfo,
                               imageConfigInfo.memPropertyFlags,
                               image,
                               imageAllocation);
}

void EngineImage::createImageView(ImageViewConfigInfo imageViewConfigInfo)
{
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
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image view");
  }
}

// taken from:
// https://github.com/emeiri/ogldev
void EngineImage::ImageMemoryBarrier(VkCommandBuffer commandBuffer,
                                     VkImage image,
                                     VkFormat format,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.baseMipLevel = 0;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;

  if ((newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ||
      (format == VK_FORMAT_D16_UNORM) ||
      (format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
      (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_S8_UINT) ||
      (format == VK_FORMAT_D16_UNORM_S8_UINT) ||
      (format == VK_FORMAT_D24_UNORM_S8_UINT))
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (HasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           newLayout == VK_IMAGE_LAYOUT_GENERAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } /* Convert back from read-only to updateable */
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } /* Convert from updateable texture to shader read-only */
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } /* Convert depth texture from undefined state to depth-stencil buffer
     */
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } /* Wait for render pass to complete */
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = 0;  // VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = 0;

    srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } /* Convert back from read-only to color attachment */
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } /* Convert from updateable texture to shader read-only */
  else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } /* Convert back from read-only to depth attachment */
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dstStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  } /* Convert from updateable depth texture to shader read-only */
  else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
  {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;

    srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  else {
    throw std::runtime_error(
        "attempting to transition to an unsupported layout "
        "configuration");
  }

  vkCmdPipelineBarrier(commandBuffer,
                       srcStage,
                       dstStage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);
}

bool EngineImage::HasStencilComponent(VkFormat format)
{
  return ((format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
          (format == VK_FORMAT_D24_UNORM_S8_UINT));
}

}  // namespace GameEngine
