#include "engine_descriptor.hpp"
#include "lib/sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "src/engine_device.hpp"

// std
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace GameEngine {

// *************** Descriptor Set Layout Builder *********************

EngineDescriptorSetLayout::Builder &
EngineDescriptorSetLayout::Builder::addBinding(uint32_t binding,
                                               VkDescriptorType descriptorType,
                                               VkShaderStageFlags stageFlags,
                                               uint32_t count) {
  assert(bindings.count(binding) == 0 && "Binding already in use");
  VkDescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.binding = binding;
  layoutBinding.descriptorType = descriptorType;
  layoutBinding.descriptorCount = count;
  layoutBinding.stageFlags = stageFlags;
  bindings[binding] = layoutBinding;
  return *this;
}

EngineDescriptorSetLayout::Builder &
EngineDescriptorSetLayout::Builder::setFlags(
    VkDescriptorSetLayoutCreateFlags flags) {
  descriptorFlags = flags;
  return *this;
}

EngineDescriptorSetLayout::Builder &
EngineDescriptorSetLayout::Builder::setpNext(const void *pNext) {
  pNextSet = pNext;
  return *this;
}

std::unique_ptr<EngineDescriptorSetLayout>
EngineDescriptorSetLayout::Builder::build() const {
  return std::make_unique<EngineDescriptorSetLayout>(geDevice, bindings,
                                                     descriptorFlags, pNextSet);
}

// *************** Descriptor Set Layout *********************

EngineDescriptorSetLayout::EngineDescriptorSetLayout(
    EngineDevice &geDevice,
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
    VkDescriptorSetLayoutCreateFlags flags, const void *pNext)
    : geDevice{geDevice}, bindings{bindings} {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
  for (auto kv : bindings) {
    setLayoutBindings.push_back(kv.second);
  }

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.flags = flags;
  descriptorSetLayoutInfo.bindingCount =
      static_cast<uint32_t>(setLayoutBindings.size());
  descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
  descriptorSetLayoutInfo.pNext = pNext;

  if (vkCreateDescriptorSetLayout(geDevice.device(), &descriptorSetLayoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

EngineDescriptorSetLayout::~EngineDescriptorSetLayout() {
  vkDestroyDescriptorSetLayout(geDevice.device(), descriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************

EngineDescriptorPool::Builder &
EngineDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType,
                                           uint32_t count) {
  poolSizes.push_back({descriptorType, count});
  return *this;
}

EngineDescriptorPool::Builder &EngineDescriptorPool::Builder::addPoolSize(
    const std::vector<VkDescriptorPoolSize> &poolSizesVec) {
  poolSizes.insert(poolSizes.end(), poolSizesVec.begin(), poolSizesVec.end());
  return *this;
}

EngineDescriptorPool::Builder &
EngineDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
  poolFlags = flags;
  return *this;
}
EngineDescriptorPool::Builder &
EngineDescriptorPool::Builder::setMaxSets(uint32_t count) {
  maxSets = count;
  return *this;
}

std::unique_ptr<EngineDescriptorPool>
EngineDescriptorPool::Builder::build() const {
  return std::make_unique<EngineDescriptorPool>(geDevice, maxSets, poolFlags,
                                                poolSizes);
}

// *************** Descriptor Pool *********************

EngineDescriptorPool::EngineDescriptorPool(
    EngineDevice &geDevice, uint32_t maxSets,
    VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes)
    : geDevice{geDevice} {
  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  descriptorPoolInfo.maxSets = maxSets;
  descriptorPoolInfo.flags = poolFlags;

  if (vkCreateDescriptorPool(geDevice.device(), &descriptorPoolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

EngineDescriptorPool::~EngineDescriptorPool() {
  vkDestroyDescriptorPool(geDevice.device(), descriptorPool, nullptr);
}

bool EngineDescriptorPool::allocateDescriptorSet(
    const VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorSet &descriptorSet, const void *pNextAlloc) const {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.pSetLayouts = &descriptorSetLayout;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pNext = pNextAlloc;

  if (vkAllocateDescriptorSets(geDevice.device(), &allocInfo, &descriptorSet) !=
      VK_SUCCESS) {
    return false;
  }
  return true;
}

void EngineDescriptorPool::freeDescriptors(
    std::vector<VkDescriptorSet> &descriptors) const {
  vkFreeDescriptorSets(geDevice.device(), descriptorPool,
                       static_cast<uint32_t>(descriptors.size()),
                       descriptors.data());
}

void EngineDescriptorPool::resetPool() {
  vkResetDescriptorPool(geDevice.device(), descriptorPool, 0);
}

// *************** Descriptor Writer *********************

EngineDescriptorWriter::EngineDescriptorWriter(
    EngineDescriptorSetLayout &setLayout,
    EngineDescriptorPoolGrowable &growablePool)
    : setLayout{setLayout}, growablePool{growablePool} {}

EngineDescriptorWriter &
EngineDescriptorWriter::writeBuffer(uint32_t binding,
                                    VkDescriptorBufferInfo *bufferInfo) {
  assert(setLayout.bindings.count(binding) == 1 &&
         "Layout does not contain specified binding");

  auto &bindingDescription = setLayout.bindings[binding];

  assert(bindingDescription.descriptorCount == 1 &&
         "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pBufferInfo = bufferInfo;
  write.descriptorCount = 1;

  writes.push_back(write);
  return *this;
}

EngineDescriptorWriter &
EngineDescriptorWriter::writeImage(uint32_t binding,
                                   VkDescriptorImageInfo *imageInfo) {
  assert(setLayout.bindings.count(binding) == 1 &&
         "Layout does not contain specified binding");

  auto &bindingDescription = setLayout.bindings[binding];

  assert(bindingDescription.descriptorCount == 1 &&
         "Binding single descriptor info, but binding expects multiple.");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = imageInfo;
  write.descriptorCount = 1;

  writes.push_back(write);
  return *this;
}

EngineDescriptorWriter &EngineDescriptorWriter::writeBulkImage(
    uint32_t binding, const std::vector<VkDescriptorImageInfo> &imagesInfo) {

  assert(setLayout.bindings.count(binding) == 1 &&
         "Layout does not contain specified binding");

  auto &bindingDescription = setLayout.bindings[binding];

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = imagesInfo.data();
  write.descriptorCount = static_cast<uint32_t>(imagesInfo.size());

  writes.push_back(write);
  return *this;
}

bool EngineDescriptorWriter::build(VkDescriptorSet &set,
                                   const void *pNextAlloc) {
  bool success = growablePool.allocateDescriptorSet(
      setLayout.getDescriptorSetLayout(), set, pNextAlloc);
  if (!success) {
    return false;
  }
  overwrite(set);
  return true;
}

void EngineDescriptorWriter::overwrite(VkDescriptorSet &set) {
  for (auto &write : writes) {
    write.dstSet = set;
  }
  vkUpdateDescriptorSets(growablePool.geDevice.device(), writes.size(),
                         writes.data(), 0, nullptr);
}

// *************** Growable Descriptor Pool Builder *********************
EngineDescriptorPoolGrowable::Builder &
EngineDescriptorPoolGrowable::Builder::addPoolSizeRatio(PoolSizeRatio ratio) {
  poolSizeRatios.push_back(ratio);
  return *this;
}

EngineDescriptorPoolGrowable::Builder &
EngineDescriptorPoolGrowable::Builder::addPoolSizeRatioVector(
    const std::vector<PoolSizeRatio> ratios) {
  poolSizeRatios = ratios;
  return *this;
}
EngineDescriptorPoolGrowable::Builder &
EngineDescriptorPoolGrowable::Builder::setNumSets(uint32_t num) {
  numSets = num;
  return *this;
}

std::unique_ptr<EngineDescriptorPoolGrowable>
EngineDescriptorPoolGrowable::Builder::build() const {
  return std::make_unique<EngineDescriptorPoolGrowable>(geDevice, numSets,
                                                        poolSizeRatios);
}

// *************** Growable Descriptor Pool *********************

EngineDescriptorPoolGrowable::EngineDescriptorPoolGrowable(
    EngineDevice &geDevice, uint32_t numSets,
    const std::vector<PoolSizeRatio> &poolSizeRatio)
    : geDevice{geDevice}, setsPerPool{numSets}, poolSizeRatios{poolSizeRatio} {

  createPool(setsPerPool);
}

void EngineDescriptorPoolGrowable::createPool(uint32_t setCount) {
  EngineDescriptorPool::Builder builder =
      EngineDescriptorPool::Builder(geDevice);

  for (auto &ratio : poolSizeRatios) {
    // assert(ratio.ratio == 0 && "Pool size ratio cannot be 0");

    builder.addPoolSize(ratio.type, uint32_t(ratio.ratio * setsPerPool))
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
  }

  std::unique_ptr<EngineDescriptorPool> newPool =
      builder.setMaxSets(setCount).build();
  readyPools.push_back(std::move(newPool));
}

VkDescriptorPool EngineDescriptorPoolGrowable::getAvailablePool() {
  checkAvailablePool();
  return readyPools.back()->getVkPool();
}

void EngineDescriptorPoolGrowable::checkAvailablePool() {
  if (readyPools.size() == 0) {
    createPool(setsPerPool);

    setsPerPool = setsPerPool * 1.5;
    if (setsPerPool > 4092)
      setsPerPool = 4092;
  }
}

bool EngineDescriptorPoolGrowable::allocateDescriptorSet(
    VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptorSet,
    const void *pNextAlloc) {

  checkAvailablePool();

  while (!readyPools.back()->allocateDescriptorSet(descriptorSetLayout,
                                                   descriptorSet, pNextAlloc)) {
    fullPools.push_back(std::move(readyPools.back()));
    readyPools.pop_back();

    checkAvailablePool();
  }
  // FIXME: handle return vals properly
  return true;
}
} // namespace GameEngine
