#pragma once

#include "engine_mesh.hpp"
#include "src/engine_device.hpp"
#include "src/engine_node.hpp"
#include "src/engine_texture.hpp"
#include "vulkan/vulkan_core.h"

#include <cstdint>
#include <filesystem>
#include <memory>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <unordered_map>
#include <vector>

namespace GameEngine {

class EngineModel {

public:
  static std::unique_ptr<EngineModel>
  createModelFromFile(EngineDevice &geDevice,
                      const std::filesystem::path &filePath);

  EngineModel(EngineDevice &geDevice, const std::filesystem::path &path);

  ~EngineModel();

  void bind(VkCommandBuffer commandBuffer);

  void draw(VkCommandBuffer commandBuffer);

  std::vector<std::shared_ptr<EngineMesh>> meshes;
  // std::unordered_map<std::string, std::unique_ptr<Node>> nodes;
  std::unordered_map<std::string, EngineTexture> images;
  // std::unordered_map<std::string, class Tp>

private:
  void loadModel(const std::filesystem::path &filePath);

  EngineDevice &geDevice;
};

} // namespace GameEngine
