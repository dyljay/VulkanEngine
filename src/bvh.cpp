#include "bvh.hpp"

#include <memory>

#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

TreeNode::TreeNode() {}
TreeNode::TreeNode(AABB& bbox)
    : bbox{bbox}
{}

BVHAccel::BVHAccel(const GameObject::Map& geObjects, EngineDevice& geDevice)
    : geDevice{geDevice}
{
  copyVertexData(geObjects);
}

BVHAccel::~BVHAccel() {}

void BVHAccel::copyVertexData(const GameObject::Map& geObjects)
{
  uint64_t totalSize;

  for (auto& kv : geObjects) {
    auto& obj = kv.second;

    if (obj.model == nullptr) continue;

    for (auto& mesh : obj.model->meshes) {
      totalSize += mesh->getVertexBuffer()->getInstanceCount() *
                   mesh->getVertexBuffer()->getBufferSize();
    }
  }

  culminatedVertexBuffer = std::make_unique<EngineBuffer>(
      geDevice,
      totalSize,
      1,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO,
      1,
      VMA_MEMORY_USAGE_GPU_ONLY);

  totalSize = 0;

  for (auto& kv : geObjects) {
    auto& obj = kv.second;

    if (obj.model == nullptr) continue;

    for (auto& mesh : obj.model->meshes) {
      uint64_t size = mesh->getVertexBuffer()->getInstanceCount() *
                      mesh->getVertexBuffer()->getInstanceSize();

      geDevice.copyBuffer(mesh->getVertexBuffer()->getBuffer(),
                          culminatedVertexBuffer->getBuffer(),
                          size,
                          0,
                          totalSize);

      totalSize += size;
    }
  }
}

TreeNode* BVHAccel::createTree(TreeNode* node, uint32_t indexCount)
{
  return new TreeNode;
}

TreeNode* BVHAccel::createTreeHelper() { return new TreeNode; }
}  // namespace GameEngine
