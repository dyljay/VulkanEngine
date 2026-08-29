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
#include "glm/fwd.hpp"
#include "primitive.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
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
  createAABBPipeline();
  createMortonCompPipeline();
  createSortingPipeline();
  createTreeGenCompPipeline();
  createMergeCompPipeline();
}

BVHAccel::~BVHAccel() {}

void BVHAccel::createAABBs(const GameObject::Map& geObjects) {}

void BVHAccel::constructTree(int index)
{
  if (VkCommandBuffer commandBuffer =
          computationManager.beginComputation(index))
  {
    resetBuffers(commandBuffer, index);

    mortonGeneration->bind(commandBuffer);

    PushConstantBVHWithBounds pushBound{};
    pushBound.numPrimitives = primitiveCount;
    pushBound.sceneDiff = glm::vec4{40.f};
    pushBound.sceneMin = glm::vec4{-20.f};

    vkCmdPushConstants(commandBuffer,
                       mortonGeneration->getPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(PushConstantBVHWithBounds),
                       &pushBound);

    mortonGeneration->dispatch(commandBuffer,
                               &mortonCodesDS[index],
                               primitiveCount);

    EngineBuffer::SetBufferPipelineBarrier(commandBuffer,
                                           mortonCodes[index]->getBuffer(),
                                           BufferAccess::ComputeWrite,
                                           BufferAccess::ComputeReadWrite);

    EngineBuffer::SetBufferPipelineBarrier(commandBuffer,
                                           primitiveIndices[index]->getBuffer(),
                                           BufferAccess::ComputeWrite,
                                           BufferAccess::ComputeReadWrite);
    PushConstantBVH push{};
    push.numPrimitives = primitiveCount;

    radixSortPipeline->bind(commandBuffer);
    vkCmdPushConstants(commandBuffer,
                       mortonGeneration->getPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(PushConstantBVHWithBounds),
                       &push);
    radixSortPipeline->dispatch(commandBuffer, &sortMortonCodesDS[index], 1);

    EngineBuffer::SetBufferPipelineBarrier(
        commandBuffer,
        sortedMortonCodes[index]->getBuffer(),
        BufferAccess::ComputeReadWrite,
        BufferAccess::ComputeRead);

    treeGeneration->bind(commandBuffer);
    vkCmdPushConstants(commandBuffer,
                       mortonGeneration->getPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(PushConstantBVHWithBounds),
                       &push);
    treeGeneration->dispatch(commandBuffer, &treeDS[index], primitiveCount - 1);

    EngineBuffer::SetBufferPipelineBarrier(
        commandBuffer,
        primitiveIndicesOut[index]->getBuffer(),
        BufferAccess::ComputeReadWrite,
        BufferAccess::ComputeRead);

    EngineBuffer::SetBufferPipelineBarrier(commandBuffer,
                                           leafNodes[index]->getBuffer(),
                                           BufferAccess::ComputeWrite,
                                           BufferAccess::ComputeRead);

    EngineBuffer::SetBufferPipelineBarrier(commandBuffer,
                                           internalNodes[index]->getBuffer(),
                                           BufferAccess::ComputeWrite,
                                           BufferAccess::ComputeReadWrite);

    mergeAABBPipeline->bind(commandBuffer);
    vkCmdPushConstants(commandBuffer,
                       mortonGeneration->getPipelineLayout(),
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(PushConstantBVHWithBounds),
                       &push);
    mergeAABBPipeline->dispatch(commandBuffer,
                                &mergeAABBDS[index],
                                primitiveCount);

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

void BVHAccel::resetBuffers(VkCommandBuffer commandBuffer, int index)
{
  // resetting the mortonCodes
  vkCmdFillBuffer(commandBuffer,
                  mortonCodes[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);
  // sorted mortonCodes buffer
  vkCmdFillBuffer(commandBuffer,
                  sortedMortonCodes[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);
  // prim indices
  vkCmdFillBuffer(commandBuffer,
                  primitiveIndices[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);

  // prim sorted
  vkCmdFillBuffer(commandBuffer,
                  primitiveIndicesOut[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);
  // leaf nodes
  vkCmdFillBuffer(commandBuffer,
                  leafNodes[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);

  // internalNodes, but also makes sure to set the root node parent to -1
  vkCmdFillBuffer(commandBuffer,
                  internalNodes[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);
  // set root node parent to -1
  vkCmdFillBuffer(commandBuffer,
                  internalNodes[index]->getBuffer(),
                  4 * sizeof(uint32_t),
                  sizeof(int32_t),
                  -1);
  // node buffer
  vkCmdFillBuffer(commandBuffer,
                  nodeAABBs[index]->getBuffer(),
                  0,
                  VK_WHOLE_SIZE,
                  0);
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
  oneBinding = EngineDescriptorSetLayout::Builder(geDevice)
                   .addBinding(0,
                               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               VK_SHADER_STAGE_COMPUTE_BIT)
                   .build();

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
  aabbGenDS.resize(NUM_BUFFER);
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

    EngineDescriptorWriter(*oneBinding, growablePool)
        .writeBuffer(0, &primbboxBufferInfo)
        .build(aabbGenDS[i]);

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

void BVHAccel::createAABBPipeline()
{
  sortPipeline =
      std::make_unique<ComputePipeline>(geDevice,
                                        "./shaders/copy_aabb.comp.spv",
                                        oneBinding->getDescriptorSetLayout(),
                                        sizeof(AABBSortPipelinePush));
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
