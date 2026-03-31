#pragma once

#include "engine_buffer.hpp"
#include "glm/fwd.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>

namespace GameEngine {

struct MaterialProperties {
  uint64_t numFeatures{0};

  // 16 bytes
  glm::vec4 baseColor{1.0f};

  // 16 bytes
  unsigned int baseColorMap{3};
  float metallic{1.0f};
  float roughness{1.0f};
  unsigned int emmissiveMap{3}; // indices for textures

  // 8 bytes
  unsigned int normalMap{3};
  unsigned int occlusionMap{3};
  unsigned int metallicRoughness{3};
};

class PBRMaterial {
public:
  enum Features {
    GLSL_HAS_BASE_MAP = 0x1 << 0x0,
    GLSL_HAS_NORMAL_MAP = 0x1 << 0x1,
    GLSL_HAS_METAL_MAP = 0x1 << 0x2
  };

  struct MaterialProperties {};

private:
  std::unique_ptr<EngineBuffer> pbrBuffer;
};
} // namespace GameEngine
