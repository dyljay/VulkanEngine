#pragma once

#include "engine_descriptor.hpp"
#include "engine_mesh.hpp"
#include "pbrMaterials.hpp"
#include "src/engine_device.hpp"
#include "src/engine_texture.hpp"
#include "vulkan/vulkan_core.h"

#include <filesystem>
#include <memory>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

namespace GameEngine {

class EngineModel {

public:
  static constexpr int MAX_SETS = 100;

  static std::unique_ptr<EngineModel>
  createModelFromFile(EngineDevice &geDevice,
                      const std::filesystem::path &filePath);

  EngineModel(EngineDevice &geDevice, const std::filesystem::path &path);

  ~EngineModel();

  std::unique_ptr<EngineDescriptorPoolGrowable> growablePool;

  void bind(VkCommandBuffer commandBuffer);

  void draw(VkCommandBuffer commandBuffer);

  std::vector<std::shared_ptr<EngineMesh>> meshes;
  std::vector<std::shared_ptr<EngineTexture>> images;
  std::vector<std::shared_ptr<MaterialProperties>> materials;

  void loadModel(const std::filesystem::path &filePath);
  void buildDescriptorPool(uint32_t numSets);
  void loadMaterials();
  void loadTextures();
  void loadVertices();

  VkFilter getMinFilter();
  VkFilter getMagFilter();

  EngineDevice &geDevice;
  fastgltf::Asset gltf;
};

} // namespace GameEngine
