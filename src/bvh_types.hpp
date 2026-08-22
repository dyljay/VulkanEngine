#pragma once

#include <cstdint>
#include <glm/common.hpp>
#include <glm/fwd.hpp>

namespace GameEngine {

struct InternalNode {
  int left = 0;
  int right = 0;
  uint32_t leftIsLeaf = 0;
  uint32_t rightIsLeaf = 0;
  int parent = -1;
  uint32_t visited = 0;
};

struct LeafNode {
  int parent;
};

struct PushConstantBVH {
  uint32_t numPrimitives;
};

struct PushConstantBVHWithBounds {
  glm::vec4 sceneMin;
  glm::vec4 sceneDiff;
  uint32_t numPrimitives;
};
}  // namespace GameEngine
