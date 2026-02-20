#pragma once

#include "engine_device.hpp"

// std
#include <memory>
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

    std::unique_ptr<EngineDescriptorSetLayout> build() const;

  private:
    EngineDevice &geDevice;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };

  EngineDescriptorSetLayout(
      EngineDevice &geDevice,
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
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

  bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout,
                          VkDescriptorSet &descriptor) const;

  void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

  void resetPool();

  VkDescriptorPool getPool() const { return descriptorPool; }

private:
  EngineDevice &geDevice;
  VkDescriptorPool descriptorPool;

  friend class EngineDescriptorWriter;
};

class EngineDescriptorWriter {
public:
  EngineDescriptorWriter(EngineDescriptorSetLayout &setLayout,
                         EngineDescriptorPool &pool);

  EngineDescriptorWriter &writeBuffer(uint32_t binding,
                                      VkDescriptorBufferInfo *bufferInfo);
  EngineDescriptorWriter &writeImage(uint32_t binding,
                                     VkDescriptorImageInfo *imageInfo);

  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);

private:
  EngineDescriptorSetLayout &setLayout;
  EngineDescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};
} // namespace GameEngine
