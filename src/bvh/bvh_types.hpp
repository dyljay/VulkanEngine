#pragma once

#include <cstdint>
#include <glm/common.hpp>
#include <glm/fwd.hpp>

namespace GameEngine {

struct InternalNode {
  int32_t left = 0;
  int32_t right = 0;
  uint32_t leftIsLeaf = 0;
  uint32_t rightIsLeaf = 0;
  int32_t parent = -1;
  uint32_t visited = 0;
};

struct LeafNode {
  int32_t parent;
};

struct PushConstantBVH {
  uint32_t numPrimitives;
};

struct PushConstantBVHWithBounds {
  glm::vec4 sceneMin;
  glm::vec4 sceneDiff;
  uint32_t numPrimitives;
};

struct AABBSortPipelinePush {
  glm::mat4 localTransform{1.f};
  glm::mat4 model{1.f};
};
}  // namespace GameEngine
