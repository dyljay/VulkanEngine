#pragma once

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"

// std
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace GameEngine {

class EngineDescriptorSetLayout {
public:
  class Builder {
  public:
    Builder(EngineDevice &geDevice) : geDevice{geDevice} {}

    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags, uint32_t count = 1);

    Builder &setFlags(VkDescriptorSetLayoutCreateFlags flags);
    Builder &setpNext(const void *pNext);

    std::unique_ptr<EngineDescriptorSetLayout> build() const;

  private:
    EngineDevice &geDevice;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    VkDescriptorSetLayoutCreateFlags descriptorFlags = 0U;
    const void *pNextSet = nullptr;
  };

  EngineDescriptorSetLayout(
      EngineDevice &geDevice,
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
      VkDescriptorSetLayoutCreateFlags flags, const void *pNext);
  ~EngineDescriptorSetLayout();
  EngineDescriptorSetLayout(const EngineDescriptorSetLayout &) = delete;
  EngineDescriptorSetLayout &
  operator=(const EngineDescriptorSetLayout &) = delete;

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  EngineDevice &geDevice;
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

  friend class EngineDescriptorWriter;
};

class EngineDescriptorPool {
public:
  class Builder {
  public:
    Builder(EngineDevice &geDevice) : geDevice{geDevice} {}
    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &addPoolSize(const std::vector<VkDescriptorPoolSize> &poolSizesVec);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<EngineDescriptorPool> build() const;

  private:
    EngineDevice &geDevice;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    uint32_t maxSets = 1000;
    VkDescriptorPoolCreateFlags poolFlags = 0;
  };

  EngineDescriptorPool(EngineDevice &geDevice, uint32_t maxSets,
                       VkDescriptorPoolCreateFlags poolFlags,
                       const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~EngineDescriptorPool();
  EngineDescriptorPool(const EngineDescriptorPool &) = delete;
  EngineDescriptorPool &operator=(const EngineDescriptorPool &) = delete;

  bool allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
                             VkDescriptorSet &descriptor,
                             const void *pNextAlloc) const;

  void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

  void resetPool();

  VkDescriptorPool getVkPool() { return descriptorPool; }

private:
  EngineDevice &geDevice;
  VkDescriptorPool descriptorPool;

  friend class EngineDescriptorWriter;
};

struct PoolSizeRatio {
  VkDescriptorType type;
  float ratio;
};

class EngineDescriptorPoolGrowable {
public:
  class Builder {
  public:
    Builder(EngineDevice &geDevice) : geDevice{geDevice} {}
    Builder &setNumSets(uint32_t numSets);
    Builder &addPoolSizeRatio(PoolSizeRatio ratio);
    Builder &addPoolSizeRatioVector(const std::vector<PoolSizeRatio> ratios);
    std::unique_ptr<EngineDescriptorPoolGrowable> build() const;

  private:
    EngineDevice &geDevice;
    std::vector<PoolSizeRatio> poolSizeRatios;
    uint32_t numSets;
  };

  EngineDescriptorPoolGrowable(EngineDevice &geDevice, uint32_t numSets,
                               const std::vector<PoolSizeRatio> &poolSizeRatio);
  VkDescriptorPool getAvailablePool();
  void createPool(uint32_t setCount);
  bool allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
                             VkDescriptorSet &descriptor,
                             const void *pNextAlloc);

  void checkAvailablePool();

private:
  EngineDevice &geDevice;

  std::vector<PoolSizeRatio> poolSizeRatios;
  std::vector<std::unique_ptr<EngineDescriptorPool>> readyPools;
  std::vector<std::unique_ptr<EngineDescriptorPool>> fullPools;

  uint32_t setsPerPool;

  friend class EngineDescriptorWriter;
};

class EngineDescriptorWriter {
public:
  EngineDescriptorWriter(EngineDescriptorSetLayout &setLayout,
                         EngineDescriptorPoolGrowable &growablePool);

  EngineDescriptorWriter &writeBuffer(uint32_t binding,
                                      VkDescriptorBufferInfo *bufferInfo);
  EngineDescriptorWriter &writeImage(uint32_t binding,
                                     VkDescriptorImageInfo *imageInfo);
  EngineDescriptorWriter &
  writeBulkImage(uint32_t binding,
                 const std::vector<VkDescriptorImageInfo> &imagesInfo);

  bool build(VkDescriptorSet &set, const void *pNextAlloc = nullptr);
  void overwrite(VkDescriptorSet &set);

private:
  EngineDescriptorSetLayout &setLayout;
  EngineDescriptorPoolGrowable &growablePool;
  std::vector<VkWriteDescriptorSet> writes;
};
} // namespace GameEngine
