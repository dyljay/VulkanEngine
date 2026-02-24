#pragma once

#include "engine_mesh.hpp"

#include <cstdint>
#include <memory>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

namespace GameEngine {

struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
};

class EngineModel {
public:
  void loadModel(const std::string &filePath);

private:
  std::vector<std::shared_ptr<EngineMesh>> meshes;
};

} // namespace GameEngine
