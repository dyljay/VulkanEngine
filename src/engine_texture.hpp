#pragma once

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <string>

namespace GameEngine {

class EngineTexture {
public:
  static constexpr int MAX_TEXTURES = 1000;

  EngineTexture(EngineDevice &geDevice, const std::string &filePath);

  ~EngineTexture();

  bool setPipelineStageFlags(VkImageMemoryBarrier &barrier,
                             const VkImageLayout oldLayout,
                             const VkImageLayout newLayout,
                             VkPipelineStageFlags &srcStage,
                             VkPipelineStageFlags &dstFlag);

  void createTextureImageView(VkFormat format);

private:
  EngineDevice &geDevice;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;

  void createTextureImage(const std::string &filePath);

  void createImage(uint32_t texWidth, uint32_t texHeight);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

  void createImageSampler();
};
} // namespace GameEngine
