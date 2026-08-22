#include "bvh.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "bvh_types.hpp"
#include "engine_buffer.hpp"
#include "engine_computer.hpp"
#include "engine_descriptor.hpp"
#include "engine_game_object.hpp"
#include "engine_pipeline.hpp"
#include "primitive.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

BVHAccel::BVHAccel(const GameObject::Map& geObjects,
                   EngineDevice& geDevice,
                   EngineDescriptorPoolGrowable& growablePool)
    : geDevice{geDevice},
      growablePool{growablePool},
      computationManager{geDevice, NUM_BUFFER}
{
  initializeAABB(geObjects);
  initializeMortonCodeBuffers();
  initializeNodeBuffers();
  createDescriptorSetLayouts();
  createDescriptorSets();
  createMortonCompPipeline();
  createSortingPipeline();
  createTreeGenCompPipeline();
  createMergeCompPipeline();
}

BVHAccel::~BVHAccel() {}

void BVHAccel::constructTree()
{
  if (VkCommandBuffer commandBuffer = computationManager.beginComputation()) {
    mortonGeneration->bind(commandBuffer);

    radixSortPipeline->bind(commandBuffer);

    computationManager.endComputation(commandBuffer);
  }
}

void BVHAccel::initializeAABB(const GameObject::Map& geObjects)
{
  primitiveAABBs.resize(NUM_BUFFER);

  for (int i = 0; i < NUM_BUFFER; i++) {
    updateAABBDataIndex(geObjects, i);
  }
}

void BVHAccel::updateAABBDataIndex(const GameObject::Map& geObjects, int index)
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

  primitiveCount = aabbVec.size();

  EngineBuffer stagingBuffer =
      EngineBuffer(geDevice,
                   sizeof(AABBPush),
                   aabbVec.size(),
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                   1,
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT);

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void*)aabbVec.data());
  stagingBuffer.unmap();

  primitiveAABBs[index] = std::make_unique<EngineBuffer>(
      geDevice,
      sizeof(AABBPush),
      aabbVec.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  geDevice.copyBuffer(stagingBuffer.getBuffer(),
                      primitiveAABBs[index]->getBuffer(),
                      sizeof(AABBPush) * aabbVec.size());
}

void BVHAccel::initializeMortonCodeBuffers()
{
  primitiveIndices.resize(NUM_BUFFER);
  primitiveIndicesOut.resize(NUM_BUFFER);
  mortonCodes.resize(NUM_BUFFER);
  sortedMortonCodes.resize(NUM_BUFFER);

  for (int i = 0; i < NUM_BUFFER; i++) {
    primitiveIndices[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(uint32_t),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    primitiveIndicesOut[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(uint32_t),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    mortonCodes[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(uint32_t),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    sortedMortonCodes[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(uint32_t),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  }
}

void BVHAccel::initializeNodeBuffers()
{
  leafNodes.resize(NUM_BUFFER);
  internalNodes.resize(NUM_BUFFER);
  nodeAABBs.resize(NUM_BUFFER);

  for (int i = 0; i < NUM_BUFFER; i++) {
    leafNodes[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(LeafNode),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    internalNodes[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(InternalNode),
        primitiveCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    // initializing internalNodes[0].parent = -1 for the root node so
    // terminating condition can be met

    EngineBuffer internalWrite{
        geDevice,
        sizeof(InternalNode),
        1,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        1,
        VMA_ALLOCATION_CREATE_MAPPED_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT};

    InternalNode rootNode{};
    internalWrite.map();
    internalWrite.writeToBuffer(&rootNode, sizeof(InternalNode), 0);
    internalWrite.unmap();

    geDevice.copyBuffer(internalWrite.getBuffer(),
                        internalNodes[i]->getBuffer(),
                        sizeof(InternalNode));

    nodeAABBs[i] = std::make_unique<EngineBuffer>(
        geDevice,
        sizeof(AABBPush),
        primitiveCount - 1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  }
}

void BVHAccel::createDescriptorSetLayouts()
{
  threeBindings = EngineDescriptorSetLayout::Builder(geDevice)
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
  fourBindings = EngineDescriptorSetLayout::Builder(geDevice)
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
  fiveBindings = EngineDescriptorSetLayout::Builder(geDevice)
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

void BVHAccel::createDescriptorSets()
{
  mortonCodesDS.resize(NUM_BUFFER);
  sortMortonCodesDS.resize(NUM_BUFFER);
  treeDS.resize(NUM_BUFFER);
  mergeAABBDS.resize(NUM_BUFFER);

  for (int i = 0; i < NUM_BUFFER; i++) {
    auto primbboxBufferInfo = primitiveAABBs[i]->descriptorInfo();
    auto mortonCodeBufferInfo = mortonCodes[i]->descriptorInfo();
    auto sortedMortonCodesBufferInfo = sortedMortonCodes[i]->descriptorInfo();
    auto primitiveIndicesBufferInfo = primitiveIndices[i]->descriptorInfo();
    auto primitiveIndicesOutBufferInfo =
        primitiveIndicesOut[i]->descriptorInfo();
    auto leafNodesBufferInfo = leafNodes[i]->descriptorInfo();
    auto internalNodesBufferInfo = internalNodes[i]->descriptorInfo();
    auto nodeAABBBufferInfo = nodeAABBs[i]->descriptorInfo();

    EngineDescriptorWriter(*threeBindings, growablePool)
        .writeBuffer(0, &primbboxBufferInfo)
        .writeBuffer(1, &mortonCodeBufferInfo)
        .writeBuffer(2, &primitiveIndicesBufferInfo)
        .build(mortonCodesDS[i]);

    EngineDescriptorWriter(*fourBindings, growablePool)
        .writeBuffer(0, &mortonCodeBufferInfo)
        .writeBuffer(1, &sortedMortonCodesBufferInfo)
        .writeBuffer(2, &primitiveIndicesBufferInfo)
        .writeBuffer(3, &primitiveIndicesOutBufferInfo)
        .build(sortMortonCodesDS[i]);

    EngineDescriptorWriter(*threeBindings, growablePool)
        .writeBuffer(0, &sortedMortonCodesBufferInfo)
        .writeBuffer(1, &internalNodesBufferInfo)
        .writeBuffer(2, &leafNodesBufferInfo)
        .build(treeDS[i]);

    EngineDescriptorWriter(*fiveBindings, growablePool)
        .writeBuffer(0, &primbboxBufferInfo)
        .writeBuffer(1, &primitiveIndicesOutBufferInfo)
        .writeBuffer(2, &internalNodesBufferInfo)
        .writeBuffer(3, &leafNodesBufferInfo)
        .writeBuffer(4, &nodeAABBBufferInfo)
        .build(mergeAABBDS[i]);
  }
}

void BVHAccel::createMortonCompPipeline()
{
  mortonGeneration =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/morton_code.comp.spv",
                                        threeBindings->getDescriptorSetLayout(),
                                        sizeof(PushConstantBVHWithBounds));
}

void BVHAccel::createSortingPipeline()
{
  radixSortPipeline =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/single_radixsort.comp.spv",
                                        fourBindings->getDescriptorSetLayout(),
                                        sizeof(PushConstantBVH));
}

void BVHAccel::createTreeGenCompPipeline()
{
  treeGeneration =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/radix_tree_build.comp.spv",
                                        threeBindings->getDescriptorSetLayout(),
                                        sizeof(PushConstantBVH));
}

void BVHAccel::createMergeCompPipeline()
{
  mergeAABBPipeline =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/aabb_propagate.comp.spv",
                                        fiveBindings->getDescriptorSetLayout(),
                                        sizeof(PushConstantBVH));
}

}  // namespace GameEngine
