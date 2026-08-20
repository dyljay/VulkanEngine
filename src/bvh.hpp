#pragma once

#include <memory>

#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_pipeline.hpp"
#include "glm/fwd.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

struct InternalNode {
  int left;
  int right;
  uint leftIsLeaf;
  uint rightIsLeaf;
  int parent;
  uint visited;
};

struct LeafNode {
  int parent;
};

struct PushConstantBVH {
  uint32_t numPrimitives;
};

class BVHAccel {
 public:
  BVHAccel(const GameObject::Map& geObjects, EngineDevice& geDevice);

  ~BVHAccel();

 private:
  void createMortonCompPipeline();
  void createSortingPipeline();
  void createTreeGenCompPipeline();
  void createMergeCompPipeline();
  void copyAABBData(const GameObject::Map& geObjects);
  void initializeMortonCodeBuffers();
  void initializeNodeBuffers();
  void createDescriptorLayouts();
  void createSemaphores();

  std::unique_ptr<EngineBuffer> primitiveAABBs;
  std::unique_ptr<EngineBuffer> mortonCodes;
  std::unique_ptr<EngineBuffer> sortedMortonCodes;
  std::unique_ptr<EngineBuffer> primitiveIndices;
  std::unique_ptr<EngineBuffer> leafNodes;
  std::unique_ptr<EngineBuffer> internalNodes;
  std::unique_ptr<EngineBuffer> nodeAABBs;

  std::unique_ptr<ComputePipeline> mortonGeneration;
  std::unique_ptr<ComputePipeline> radixSortPipeline;
  std::unique_ptr<ComputePipeline> treeGeneration;
  std::unique_ptr<ComputePipeline> mergeAABBPipeline;

  VkSemaphore mortonCodesGenerated;
  VkSemaphore mortonCodesSorted;
  VkSemaphore internalNodesCreated;

  VkFence canDraw;

  EngineDevice& geDevice;

  VkDescriptorSet primitiveAABBsBuffer;
  VkDescriptorSet mortonCodesBuffer;
  VkDescriptorSet sortedMortonCodesBuffer;
  VkDescriptorSet primitiveIndicesBuffer;
  VkDescriptorSet leafNodesBuffer;
  VkDescriptorSet internalNodesBuffer;
  VkDescriptorSet nodeAABBBuffer;

  uint32_t primitiveCount;
  glm::vec3 sceneMinBound{std::numeric_limits<float>::max()};
  glm::vec3 sceneDiffBound{std::numeric_limits<float>::lowest()};
};
}  // namespace GameEngine
