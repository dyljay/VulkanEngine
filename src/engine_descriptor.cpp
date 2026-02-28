#include "engine_descriptor.hpp"
#include "lib/sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "src/engine_device.hpp"

// std
#include <cassert>
#include <cstdint>
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

std::unique_ptr<EngineDescriptorSetLayout>
EngineDescriptorSetLayout::Builder::build() const {
  return std::make_unique<EngineDescriptorSetLayout>(geDevice, bindings);
}

// *************** Descriptor Set Layout *********************

EngineDescriptorSetLayout::EngineDescriptorSetLayout(
    EngineDevice &geDevice,
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : geDevice{geDevice}, bindings{bindings} {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
  for (auto kv : bindings) {
    setLayoutBindings.push_back(kv.second);
  }

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount =
      static_cast<uint32_t>(setLayoutBindings.size());
  descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

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

// should probably be called allocateDescriptorSet
bool EngineDescriptorPool::allocateDescriptor(
    const VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorSet &descriptor) const {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.pSetLayouts = &descriptorSetLayout;
  allocInfo.descriptorSetCount = 1;

  // Might want to create a "DescriptorPoolManager" class that handles this
  // case, and builds a new pool whenever an old pool fills up. But this is
  // beyond our current scope
  if (vkAllocateDescriptorSets(geDevice.device(), &allocInfo, &descriptor) !=
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
    EngineDescriptorSetLayout &setLayout, EngineDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

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
         "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = imageInfo;
  write.descriptorCount = 1;

  writes.push_back(write);
  return *this;
}

bool EngineDescriptorWriter::build(VkDescriptorSet &set) {
  bool success =
      pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
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
  vkUpdateDescriptorSets(pool.geDevice.device(), writes.size(), writes.data(),
                         0, nullptr);
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

EngineDescriptorPoolGrowable
EngineDescriptorPoolGrowable::Builder::build() const {
  return EngineDescriptorPoolGrowable{geDevice, numSets, poolSizeRatios};
}

// *************** Growable Descriptor Pool *********************

EngineDescriptorPoolGrowable::EngineDescriptorPoolGrowable(
    EngineDevice &geDevice, uint32_t numSets,
    const std::vector<PoolSizeRatio> &poolSizeRatio)
    : geDevice{geDevice}, setsPerPool{numSets}, poolSizeRatios{poolSizeRatio} {}

void EngineDescriptorPoolGrowable::createPool(uint32_t setCount) {
  std::unique_ptr<EngineDescriptorPool> newPool;

  for (auto &ratio : poolSizeRatios) {
    newPool = EngineDescriptorPool::Builder(geDevice)
                  .addPoolSize(ratio.type, uint32_t(ratio.ratio * setsPerPool))
                  .setMaxSets(setCount)
                  .build();
  }

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

void EngineDescriptorPoolGrowable::allocateDescriptorSet(
    VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptorSet) {

  checkAvailablePool();

  while (!readyPools.back()->allocateDescriptor(descriptorSetLayout,
                                                descriptorSet)) {
    fullPools.push_back(std::move(readyPools.back()));
    readyPools.pop_back();

    checkAvailablePool();
  }
}
} // namespace GameEngine
