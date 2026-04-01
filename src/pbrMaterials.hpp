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
  unsigned int numFeatures{0};
  unsigned int baseColorMap{0};
  float metallic{1.0f};
  float roughness{1.0f};

  // 12 bytes
  unsigned int normalMap{0};
  unsigned int occlusionMap{0};
  unsigned int metallicRoughness{0};
};

class PBRMaterial {
public:
  enum Features {
    GLSL_HAS_BASE_MAP = 0x1 << 0x0,
    GLSL_HAS_NORMAL_MAP = 0x1 << 0x1,
    GLSL_HAS_METAL_MAP = 0x1 << 0x2
  };

  MaterialProperties properties{};

private:
  std::unique_ptr<EngineBuffer> pbrBuffer;
};
} // namespace GameEngine
