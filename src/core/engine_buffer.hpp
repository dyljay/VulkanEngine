#pragma once

#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

/**
 * How a buffer is being accessed on either side of a pipeline barrier. Buffers
 * have no layouts, so this plays the role VkImageLayout plays for images: the
 * src value describes the work that must finish, the dst value the work that
 * must wait.
 */
enum class BufferAccess {
  None,
  HostRead,
  HostWrite,
  TransferSrc,
  TransferDst,
  ComputeRead,
  ComputeWrite,
  ComputeReadWrite,
  VertexShaderRead,
  FragmentShaderRead,
  VertexInput,
  IndexInput,
  IndirectCommand,
};

class EngineBuffer {
 public:
  EngineBuffer(EngineDevice& device,
               VkDeviceSize instanceSize,
               uint32_t instanceCount,
               VkBufferUsageFlags usageFlags,
               VmaMemoryUsage memoryUsage,
               VkDeviceSize minOffsetAlignment = 1,
               VmaAllocationCreateFlags vmaFlags = 0);

  ~EngineBuffer();

  EngineBuffer(const EngineBuffer&) = delete;
  EngineBuffer& operator=(const EngineBuffer&) = delete;

  void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void unmap();

  void writeToBuffer(void* data,
                     VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);
  VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE,
                                        VkDeviceSize offset = 0);
  VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE,
                      VkDeviceSize offset = 0);

  void writeToIndex(void* data, int index);
  VkResult flushIndex(int index);
  VkDescriptorBufferInfo descriptorInfoForIndex(int index);
  VkResult invalidateIndex(int index);

  VkBuffer getBuffer() const { return buffer; }
  void* getMappedMemory() const { return mapped; }
  uint32_t getInstanceCount() const { return instanceCount; }
  VkDeviceSize getInstanceSize() const { return instanceSize; }
  VkDeviceSize getAlignmentSize() const { return instanceSize; }
  VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
  VmaMemoryUsage getMemoryPropertyFlags() const { return memoryUsage; }
  VkDeviceSize getBufferSize() const { return bufferSize; }
  VmaAllocation getAllocation() const { return allocation_; }

  static void SetBufferPipelineBarrier(VkCommandBuffer commandBuffer,
                                       VkBuffer buffer,
                                       BufferAccess oldAccess,
                                       BufferAccess newAccess,
                                       VkDeviceSize offset = 0,
                                       VkDeviceSize size = VK_WHOLE_SIZE);

 private:
  static VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                                   VkDeviceSize minOffsetAlignment);

  static void GetAccessInfo(BufferAccess access,
                            bool isSource,
                            VkPipelineStageFlags& stage,
                            VkAccessFlags& accessMask);

  EngineDevice& lveDevice;
  void* mapped = nullptr;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  // Allocated Buffer Struct
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation_;
  VmaAllocationInfo allocInfo;

  VkDeviceSize bufferSize;
  uint32_t instanceCount;
  VkDeviceSize instanceSize;
  VkDeviceSize alignmentSize;
  VkBufferUsageFlags usageFlags;
  VmaMemoryUsage memoryUsage;
};

}  // namespace GameEngine
