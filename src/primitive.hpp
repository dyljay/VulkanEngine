#pragma once

#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"

namespace GameEngine {

struct Primitive {};

struct AABBPush {
  glm::vec4 min;
  glm::vec4 max;
};

struct AABB : Primitive {
  glm::vec4 min = glm::vec4{glm::vec3{std::numeric_limits<float>::max()}, 1.f};
  glm::vec4 max =
      glm::vec4{glm::vec3{std::numeric_limits<float>::lowest()}, 1.f};

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

  AABBPush worldSpaceBounds(glm::mat4 transform)
  {
    glm::vec3 localMin = glm::vec3(min);
    glm::vec3 localMax = glm::vec3(max);

    glm::vec3 localCenter = 0.5f * (localMax + localMin);
    glm::vec3 localExtent = 0.5f * (localMax - localMin);

    glm::vec3 worldCenter =
        glm::vec3((transform * glm::vec4(localCenter, 1.0)));

    glm::mat3 basis = glm::mat3(transform);
    glm::mat3 absBasis = glm::mat3(abs(basis[0]), abs(basis[1]), abs(basis[2]));
    glm::vec3 worldExtent = absBasis * localExtent;

    glm::vec3 worldMin = worldCenter - worldExtent;
    glm::vec3 worldMax = worldCenter + worldExtent;

    return AABBPush{.min = glm::vec4{worldMin, 1.0f},
                    .max = glm::vec4{worldMax, 1.0f}};
  }
};

}  // namespace GameEngine
