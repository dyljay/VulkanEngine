#pragma once

#include "engine_device.hpp"
#include "lib/stb_image.h"
#include "vulkan/vulkan_core.h"
#include <array>
#include <cstdint>
#include <string>

namespace GameEngine {

enum class TextureType { Texture2D, TextureCube };

class EngineTexture {
public:
  static constexpr int MAX_TEXTURES = 1000;

  EngineTexture(EngineDevice &geDevice, const std::string &filePath);

  EngineTexture(EngineDevice &geDevice,
                const std::array<std::string, 6> &facePaths);

  ~EngineTexture();

  VkImageView getImageView() const { return textureImageView; }
  VkSampler getSampler() const { return textureSampler; }

private:
  EngineDevice &geDevice;
  VkImage textureImage;
  VmaAllocation allocation;
  VkImageView textureImageView;
  VkSampler textureSampler;

  TextureType type;

  uint32_t texWidth;
  uint32_t texHeight;

  void createImageView(VkImageViewType viewType, uint32_t layers,
                       VkFormat format);
  void createSampler(VkSamplerAddressMode addressMode);

  void upload2D(const std::string &filePath);
  void uploadCube(const std::array<std::string, 6> &cubePaths);

  void createVkImage(uint32_t texWidth, uint32_t texHeight, uint32_t layers,
                     VkImageCreateFlags flags);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);

  bool setPipelineStageFlags(VkImageMemoryBarrier &barrier,
                             const VkImageLayout oldLayout,
                             const VkImageLayout newLayout,
                             VkPipelineStageFlags &srcStage,
                             VkPipelineStageFlags &dstFlag);
};
} // namespace GameEngine
