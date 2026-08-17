#pragma once

#include <memory>

#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "primitive.hpp"

namespace GameEngine {

class TreeNode {
 public:
  TreeNode(AABB& bbox);
  TreeNode();

 private:
  AABB bbox;
};

class BVHAccel {
 public:
  BVHAccel(const GameObject::Map& geObjects, EngineDevice& geDevice);

  ~BVHAccel();

 private:
  TreeNode* createTree(TreeNode* node, uint32_t indexCount);
  TreeNode* createTreeHelper();

  void copyVertexData(const GameObject::Map& geObjects);

  std::unique_ptr<EngineBuffer> culminatedVertexBuffer;
  TreeNode* root;

  EngineDevice& geDevice;
};
}  // namespace GameEngine
