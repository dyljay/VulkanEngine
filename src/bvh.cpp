#include "bvh.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "engine_buffer.hpp"
#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_pipeline.hpp"
#include "fastgltf/types.hpp"
#include "primitive.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

BVHAccel::BVHAccel(const GameObject::Map& geObjects, EngineDevice& geDevice)
    : geDevice{geDevice}
{
  copyAABBData(geObjects);
  initializeMortonCodeBuffers();
  initializeNodeBuffers();
  createMortonCompPipeline();
  createSortingPipeline();
  createTreeGenCompPipeline();
  createMergeCompPipeline();
}

BVHAccel::~BVHAccel() {}

void BVHAccel::createMortonCompPipeline()
{
  auto descriptorSetLayout = EngineDescriptorSetLayout::Builder(geDevice)
                                 .addBinding(0,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(1,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(2,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .build();

  mortonGeneration = std::make_unique<ComputePipeline>(
      geDevice,
      "./shaders/morton_code.comp",
      descriptorSetLayout->getDescriptorSetLayout());
}

void BVHAccel::createSortingPipeline()
{
  auto descriptorSetLayout = EngineDescriptorSetLayout::Builder(geDevice)
                                 .addBinding(0,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(1,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(2,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(3,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .build();

  radixSortPipeline = std::make_unique<ComputePipeline>(
      geDevice,
      "./shaders/single_radixsort.comp",
      descriptorSetLayout->getDescriptorSetLayout());
}

void BVHAccel::createTreeGenCompPipeline()
{
  auto descriptorSetLayout = EngineDescriptorSetLayout::Builder(geDevice)
                                 .addBinding(0,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(1,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(2,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .build();

  treeGeneration = std::make_unique<ComputePipeline>(
      geDevice,
      "./shaders/radix_tree_build.comp",
      descriptorSetLayout->getDescriptorSetLayout());
}

void BVHAccel::createMergeCompPipeline()
{
  auto descriptorSetLayout = EngineDescriptorSetLayout::Builder(geDevice)
                                 .addBinding(0,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(1,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(2,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(3,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .addBinding(4,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_SHADER_STAGE_COMPUTE_BIT)
                                 .build();

  mergeAABBPipeline = std::make_unique<ComputePipeline>(
      geDevice,
      "./shaders/aabb_propagate.comp",
      descriptorSetLayout->getDescriptorSetLayout());
}

void BVHAccel::copyAABBData(const GameObject::Map& geObjects)
{
  std::vector<AABBPush> aabbVec;

  for (auto& kv : geObjects) {
    auto& obj = kv.second;

    if (obj.model == nullptr) continue;

    for (auto& mesh : obj.model->meshes) {
      aabbVec.emplace_back(
          AABBPush{.min = mesh->getBBox().min, .max = mesh->getBBox().max});
    }
  }

  EngineBuffer stagingBuffer = EngineBuffer(geDevice,
                                            sizeof(AABBPush),
                                            aabbVec.size(),
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void*)aabbVec.data());

  primitiveAABBs = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(AABBPush),
      aabbVec.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  geDevice.copyBuffer(stagingBuffer.getBuffer(),
                      primitiveAABBs->getBuffer(),
                      sizeof(AABBPush) * aabbVec.size());
}

void BVHAccel::initializeMortonCodeBuffers()
{
  primitiveIndices = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(uint32_t),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  primitiveIndicesOut = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(uint32_t),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  mortonCodes = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(uint32_t),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  sortedMortonCodes = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(uint32_t),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
}

void BVHAccel::initializeNodeBuffers()
{
  leafNodes = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(LeafNode),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  internalNodes = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(InternalNode),
      primitiveCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  // initializing internalNodes[0].parent = -1 for the root node so terminating
  // condition can be met
  EngineBuffer internalWrite{geDevice,
                             sizeof(InternalNode),
                             1,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VMA_MEMORY_USAGE_AUTO_PREFER_HOST};
  InternalNode rootNode{};
  rootNode.parent = -1;
  internalWrite.map();
  internalWrite.writeToBuffer(&rootNode);
  internalWrite.unmap();

  geDevice.copyBuffer(internalNodes->getBuffer(),
                      internalWrite.getBuffer(),
                      sizeof(InternalNode));

  nodeAABBs = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(AABBPush),
      primitiveCount - 1,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
}

void BVHAccel::createSemaphores() {}

}  // namespace GameEngine
