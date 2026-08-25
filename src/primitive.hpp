#pragma once

#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

namespace GameEngine {

struct Primitive {};

struct AABB : Primitive {
  glm::vec4 min{std::numeric_limits<float>::max()};
  glm::vec4 max{std::numeric_limits<float>::lowest()};

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

  bool doesIntersect(const AABB& bbox)
  {
    return (min.x <= bbox.max.x && max.x >= bbox.min.x) &&
           (min.y <= bbox.max.y && max.y >= bbox.min.y) &&
           (min.z <= bbox.max.z && max.z >= bbox.min.z);
  }
};

struct AABBPush {
  glm::vec4 min;
  glm::vec4 max;
};

}  // namespace GameEngine
