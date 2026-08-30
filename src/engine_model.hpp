#pragma once

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>

#include "engine_descriptor.hpp"
#include "engine_mesh.hpp"
#include "engine_node.hpp"
#include "pbrMaterials.hpp"
#include "primitive.hpp"
#include "src/engine_device.hpp"
#include "src/engine_texture.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

class EngineModel {
 public:
  static constexpr int MAX_SETS = 100;

  static std::unique_ptr<EngineModel> createModelFromFile(
      EngineDevice& geDevice,
      const std::filesystem::path& filePath,
      const std::string modelName,
      EngineDescriptorPoolGrowable& growablePool);

  EngineModel(EngineDevice& geDevice,
              const std::filesystem::path& path,
              const std::string modelName,
              EngineDescriptorPoolGrowable& growablePool);

  ~EngineModel();

  const std::string& getName() const { return modelName; }

  void bind(VkCommandBuffer commandBuffer);

  void draw(VkCommandBuffer commandBuffer);

  std::vector<std::shared_ptr<EngineMesh>> meshes;
  std::vector<std::shared_ptr<EngineTexture>> images;
  std::vector<std::shared_ptr<PBRMaterial>> materials;
  std::vector<std::shared_ptr<Node>> nodes;
  std::vector<std::shared_ptr<Node>> topNodes;

 private:
  void loadModel(const std::filesystem::path& filePath);
  void buildDescriptorPool(uint32_t numSets);
  void loadMaterials(fastgltf::Asset& gltf);
  void loadTextures(fastgltf::Asset& gltf);
  void loadVertices(fastgltf::Asset& gltf);
  void loadNodes(fastgltf::Asset& gltf);
  void updateNodes();

  VkFilter getFilter(fastgltf::Filter filter);
  VkSamplerMipmapMode getSampler(fastgltf::Filter filter);

  EngineDevice& geDevice;

  EngineDescriptorPoolGrowable& growablePool;
  const std::string modelName;
};

}  // namespace GameEngine
