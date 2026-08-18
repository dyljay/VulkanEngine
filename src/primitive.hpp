#pragma once

#include <iostream>
#include <limits>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

namespace GameEngine {

struct Primitive {};

struct AABB : Primitive {
  glm::vec3 min{std::numeric_limits<float>::max()};
  glm::vec3 max{std::numeric_limits<float>::lowest()};

  void testBounds(const glm::vec3& bounds)
  {
    if (bounds.x < min.x) {
      min.x = bounds.x;
    }
    if (bounds.y < min.y) {
      min.y = bounds.y;
    }
    if (bounds.z < min.z) {
      min.z = bounds.z;
    }

    if (bounds.x > max.x) {
      max.x = bounds.x;
    }
    if (bounds.y > max.y) {
      max.y = bounds.y;
    }
    if (bounds.z > max.z) {
      max.z = bounds.z;
    }
  }

  void expandBBox(const AABB& bbox)
  {
    min = glm::min(min, bbox.min);
    max = glm::max(max, bbox.max);
  }

  bool doesIntersect(const AABB& bbox)
  {
    return (min.x <= bbox.max.x && max.x >= bbox.min.x) &&
           (min.y <= bbox.max.y && max.y >= bbox.min.y) &&
           (min.z <= bbox.max.z && max.z >= bbox.min.z);
  }
};

struct AABBPush {
  glm::vec3 min;
  float padding0;
  glm::vec3 max;
  float padding1;
};

struct SphereBound : Primitive {};

}  // namespace GameEngine
