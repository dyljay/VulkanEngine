#pragma once

#include "engine_device.hpp"
#include "lib/stb_image.h"
#include "vulkan/vulkan_core.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace GameEngine {
using id_t = unsigned int;

class EngineTexture {
public:
  enum class TextureType { Texture2D, TextureCube };
  static constexpr int MAX_TEXTURES = 100;

  static std::shared_ptr<EngineTexture> createTexture(EngineDevice &geDevice);
  static std::unique_ptr<EngineTexture>
  createCubeMap(EngineDevice &geDevice,
                const std::array<std::string, 6> &facePaths);

  EngineTexture(EngineDevice &geDevice, id_t ID);

  EngineTexture(EngineDevice &geDevice,
                const std::array<std::string, 6> &facePaths);

  ~EngineTexture();

  id_t getID() const { return id; }

  void Init(int w, int h, VkFormat imageFormat, stbi_uc *pixels,
            VkFilter minFilter, VkFilter magFilter);

  VkDescriptorImageInfo getDescriptorInfo();
  VkImageView getImageView() const { return textureImageView; }
  VkSampler getSampler() const { return textureSampler; }

private:
  id_t id;

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
  void createSampler(VkSamplerAddressMode addressMode,
                     VkFilter minFilter = VK_FILTER_LINEAR,
                     VkFilter magFilter = VK_FILTER_LINEAR);

  void upload2D(int w, int h, stbi_uc *pixels);
  void uploadCube(const std::array<std::string, 6> &cubePaths);

  void createVkImage(uint32_t texWidth, uint32_t texHeight, uint32_t layers,
                     VkImageCreateFlags flags);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t layerCount);

  bool setPipelineStageFlags(VkImageMemoryBarrier &barrier,
                             const VkImageLayout oldLayout,
                             const VkImageLayout newLayout,
                             VkPipelineStageFlags &srcStage,
                             VkPipelineStageFlags &dstFlag);
};
} // namespace GameEngine
