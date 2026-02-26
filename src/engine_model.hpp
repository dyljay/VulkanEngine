#pragma once

#include "engine_mesh.hpp"
#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"

#include <cstdint>
#include <filesystem>
#include <memory>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
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

private:
  void loadModel(const std::filesystem::path &filePath);

  // std::vector<std::shared_ptr<EngineMesh>> meshes;

  EngineDevice &geDevice;
};

} // namespace GameEngine
