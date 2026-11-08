#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "engine_computer.hpp"
#include "engine_descriptor.hpp"
#include "engine_game_object.hpp"
#include "engine_pipeline.hpp"
#include "engine_swapchain.hpp"
#include "glm/fwd.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

class BVHAccel {
  static constexpr int NUM_BUFFER = EngineSwapChain::MAX_FRAMES_IN_FLIGHT;

 public:
  BVHAccel(GameObject::Map& geObjects,
           EngineDevice& geDevice,
           EngineDescriptorPoolGrowable& growablePool);

  ~BVHAccel();

  void constructTree(int index);

  bool doesIntersect();

  void inOrderTraversal();

 private:
  void initializeAABB();
  void traverseAndUpdateAABB(std::vector<AABBPush>& aabbVec);
  void initializeMortonCodeBuffers();
  void initializeNodeBuffers();
  void createDescriptorSetLayouts();
  void createDescriptorSets();
  void createMortonCompPipeline();
  void createSortingPipeline();
  void createTreeGenCompPipeline();
  void createMergeCompPipeline();
  void updateAABBDataIndex(int index);
  void createAABBPipeline();
  void resetBuffers(VkCommandBuffer commandBuffer, int index);
  void copyAABBBuffer(int index);

  std::vector<std::unique_ptr<EngineBuffer>> primitiveAABBs;
  std::vector<std::unique_ptr<EngineBuffer>> mortonCodes;
  std::vector<std::unique_ptr<EngineBuffer>> sortedMortonCodes;
  std::vector<std::unique_ptr<EngineBuffer>> primitiveIndices;
  std::vector<std::unique_ptr<EngineBuffer>> primitiveIndicesOut;
  std::vector<std::unique_ptr<EngineBuffer>> leafNodes;
  std::vector<std::unique_ptr<EngineBuffer>> internalNodes;
  std::vector<std::unique_ptr<EngineBuffer>> nodeAABBs;

  std::unique_ptr<ComputePipeline> sortPipeline;
  std::unique_ptr<ComputePipeline> mortonGeneration;
  std::unique_ptr<ComputePipeline> radixSortPipeline;
  std::unique_ptr<ComputePipeline> treeGeneration;
  std::unique_ptr<ComputePipeline> mergeAABBPipeline;

  std::unique_ptr<EngineDescriptorSetLayout> oneBinding;
  std::unique_ptr<EngineDescriptorSetLayout> threeBindings;
  std::unique_ptr<EngineDescriptorSetLayout> fourBindings;
  std::unique_ptr<EngineDescriptorSetLayout> fiveBindings;

  std::vector<VkDescriptorSet> aabbGenDS;
  std::vector<VkDescriptorSet> mortonCodesDS;
  std::vector<VkDescriptorSet> sortMortonCodesDS;
  std::vector<VkDescriptorSet> treeDS;
  std::vector<VkDescriptorSet> mergeAABBDS;

  uint32_t primitiveCount;
  glm::vec3 sceneMinBound{std::numeric_limits<float>::max()};
  glm::vec3 sceneDiffBound{std::numeric_limits<float>::lowest()};

  std::vector<std::vector<AABBPush>> aabbVectors;

  EngineDevice& geDevice;
  EngineDescriptorPoolGrowable& growablePool;
  GameObject::Map& geObjects;
  EngineComputer computationManager;
};
}  // namespace GameEngine
