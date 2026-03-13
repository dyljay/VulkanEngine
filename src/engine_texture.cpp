#include "engine_buffer.hpp"
#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <array>
#include <cstdint>
#include <memory>
#include <sys/types.h>

#include "engine_texture.hpp"

#include <stdexcept>

namespace GameEngine {

EngineTexture::EngineTexture(EngineDevice &geDevice)
    : geDevice{geDevice}, type{TextureType::Texture2D} {}

EngineTexture::EngineTexture(EngineDevice &geDevice,
                             const std::array<std::string, 6> &cubePaths)
    : geDevice{geDevice}, type{TextureType::TextureCube} {
  uploadCube(cubePaths);
  createImageView(VK_IMAGE_VIEW_TYPE_CUBE, 6, VK_FORMAT_R8G8B8A8_SRGB);
  createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

EngineTexture::~EngineTexture() {
  vkDestroySampler(geDevice.device(), textureSampler, nullptr);

  vkDestroyImageView(geDevice.device(), textureImageView, nullptr);

  vmaDestroyImage(geDevice.getAllocator(), textureImage, allocation);
}

VkDescriptorImageInfo EngineTexture::getDescriptorInfo() {
  return VkDescriptorImageInfo{textureSampler, textureImageView,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}
void EngineTexture::drawCubeMap(VkCommandBuffer commandBuffer) {
  vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

std::shared_ptr<EngineTexture>
EngineTexture::createTexture(EngineDevice &geDevice) {
  return std::make_shared<EngineTexture>(geDevice);
}

void EngineTexture::Init(int w, int h, VkFormat imageFormat, stbi_uc *pixels,
                         VkFilter minFilter, VkFilter magFilter) {

  upload2D(w, h, pixels);
  createImageView(VK_IMAGE_VIEW_TYPE_2D, 1, imageFormat);
  createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, minFilter, magFilter);
}

void EngineTexture::upload2D(int w, int h, stbi_uc *pixels) {
  texWidth = static_cast<uint32_t>(w);
  texHeight = static_cast<uint32_t>(h);
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  EngineBuffer stagingBuffer{geDevice, imageSize, 1,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VMA_MEMORY_USAGE_CPU_TO_GPU};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)pixels);
  stagingBuffer.unmap();

  createVkImage(texWidth, texHeight, 1, 0);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

  geDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, texWidth,
                             texHeight, 1);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
}

void EngineTexture::uploadCube(const std::array<std::string, 6> &cubeArray) {
  struct Faces {
    stbi_uc *pixels;
    int w;
    int h;
  };

  std::array<Faces, 6> faces;
  for (int i = 0; i < 6; i++) {
    int nC;
    faces[i].pixels = stbi_load(cubeArray[i].c_str(), &faces[i].w, &faces[i].h,
                                &nC, STBI_rgb_alpha);

    if (!faces[i].pixels) {
      throw std::runtime_error("failed to load texture image!");
    }
  }

  texWidth = static_cast<uint32_t>(faces[0].w);
  texHeight = static_cast<uint32_t>(faces[0].h);

  VkDeviceSize imageSize = texWidth * texHeight * 4;
  VkDeviceSize totalSize = imageSize * 6;

  EngineBuffer stagingBuffer{geDevice, totalSize, 1,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VMA_MEMORY_USAGE_CPU_TO_GPU};

  stagingBuffer.map();

  for (int i = 0; i < 6; i++) {
    stagingBuffer.writeToBuffer((void *)faces[i].pixels, imageSize,
                                imageSize * i);

    stbi_image_free(faces[i].pixels);
  }
  stagingBuffer.unmap();

  createVkImage(texWidth, texHeight, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);

  geDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, texWidth,
                             texHeight, 6);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);
}

// TODO: What does the usage flag do?
void EngineTexture::createVkImage(uint32_t texWidth, uint32_t texHeight,
                                  uint32_t layers, VkImageCreateFlags flags) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = texWidth;
  imageInfo.extent.height = texHeight;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = layers;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = flags;

  geDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               textureImage, allocation);
}

void EngineTexture::transitionImageLayout(VkImage image, VkFormat format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          uint32_t layerCount) {
  if (VkCommandBuffer commandBuffer = geDevice.beginSingleTimeCommands()) {

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (!setPipelineStageFlags(barrier, oldLayout, newLayout, srcStage,
                               dstStage)) {
      throw std::runtime_error("unsupported image layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    geDevice.endSingleTimeCommands(commandBuffer);
  }
}
bool EngineTexture::setPipelineStageFlags(VkImageMemoryBarrier &barrier,
                                          const VkImageLayout oldLayout,
                                          const VkImageLayout newLayout,
                                          VkPipelineStageFlags &srcStage,
                                          VkPipelineStageFlags &dstStage) {

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    return false;
  }

  return true;
}

void EngineTexture::createImageView(VkImageViewType viewType, uint32_t layers,
                                    VkFormat format) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = textureImage;
  viewInfo.viewType = viewType;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = layers;

  if (vkCreateImageView(geDevice.device(), &viewInfo, nullptr,
                        &textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image view!");
  }
}

void EngineTexture::createSampler(VkSamplerAddressMode addressmode,
                                  VkFilter minFilter, VkFilter magFilter) {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = magFilter;
  samplerInfo.minFilter = minFilter;
  samplerInfo.addressModeU = addressmode;
  samplerInfo.addressModeV = addressmode;
  samplerInfo.addressModeW = addressmode;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = geDevice.properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(geDevice.device(), &samplerInfo, nullptr,
                      &textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}
} // namespace GameEngine
