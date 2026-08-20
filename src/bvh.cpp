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
#include "primitive.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

BVHAccel::BVHAccel(const GameObject::Map& geObjects, EngineDevice& geDevice)
    : geDevice{geDevice}
{}

BVHAccel::~BVHAccel() {}

// TODO: come back and fix after finishing the compute pipeline class
void BVHAccel::createComputePipelines()
{
  mortonGeneration =
      std::make_unique<ComputePipeline>(geDevice, "./shaders/morton_code.comp");

  radixSortPipeline =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/single_radixsort.comp");

  treeGeneration =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/radix_tree_build.comp");

  mergeAABBPipeline =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/aabb_propagate.comp");
}

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
                                 .build();
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
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

  nodeAABBs = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(AABBPush),
      primitiveCount + primitiveCount - 1,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
}

void BVHAccel::createSemaphores() {}

}  // namespace GameEngine
