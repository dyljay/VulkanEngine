#pragma once

#include "engine_buffer.hpp"
#include "glm/fwd.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace GameEngine {

struct MaterialProperties {
  // 16 bytes
  glm::vec4 baseColor{1.0f};

  // 16 bytes
  float metallic{1.0f};
  float roughness{1.0f};
  unsigned int emmissiveMap{0}; // indices for textures
  unsigned int occlusionMap{0};

  // 4 bytes
  unsigned int normalMap{0};
};

} // namespace GameEngine
