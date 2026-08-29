#include "engine_buffer.hpp"

#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"

#define VMA_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include "vk_mem_alloc.hpp"
#pragma clang diagnostic pop

// std
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace GameEngine {

/**
 * Returns the minimum instance size required to be compatible with devices
 * minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for
 * the offset member (eg minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
VkDeviceSize EngineBuffer::getAlignment(VkDeviceSize instanceSize,
                                        VkDeviceSize minOffsetAlignment)
{
  if (minOffsetAlignment > 0) {
    return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
  }
  return instanceSize;
}

EngineBuffer::EngineBuffer(EngineDevice& device,
                           VkDeviceSize instanceSize,
                           uint32_t instanceCount,
                           VkBufferUsageFlags usageFlags,
                           VmaMemoryUsage memoryUsage,
                           VkDeviceSize minOffsetAlignment,
                           VmaAllocationCreateFlags vmaFlags)
    : lveDevice{device},
      instanceSize{instanceSize},
      instanceCount{instanceCount},
      usageFlags{usageFlags},
      memoryUsage{memoryUsage}
{
  alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
  bufferSize = alignmentSize * instanceCount;
  device.createBuffer(bufferSize,
                      usageFlags,
                      memoryUsage,
                      buffer,
                      allocation_,
                      allocInfo,
                      vmaFlags);
}

EngineBuffer::~EngineBuffer()
{
  unmap();
  // handles both freeing memory and destroying buffer
  vmaDestroyBuffer(lveDevice.getAllocator(), buffer, allocation_);
}

/**
 * Maps a BufferAccess onto the pipeline stage it happens in and the access
 * mask that describes it.
 *
 * @param access The access to describe
 * @param isSource True when this is the src half of the barrier, false for dst
 * @param stage Out param, the pipeline stage the access happens in
 * @param accessMask Out param, the memory accesses to make available/visible
 */
void EngineBuffer::GetAccessInfo(BufferAccess access,
                                 bool isSource,
                                 VkPipelineStageFlags& stage,
                                 VkAccessFlags& accessMask)
{
  switch (access) {
    case BufferAccess::None:
      /* nothing to wait on, or nothing waiting */
      stage = isSource ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                       : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      accessMask = 0;
      break;
    case BufferAccess::HostRead:
      stage = VK_PIPELINE_STAGE_HOST_BIT;
      accessMask = VK_ACCESS_HOST_READ_BIT;
      break;
    case BufferAccess::HostWrite:
      stage = VK_PIPELINE_STAGE_HOST_BIT;
      accessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;
    case BufferAccess::TransferSrc:
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      accessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case BufferAccess::TransferDst:
      stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case BufferAccess::ComputeRead:
      stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      accessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case BufferAccess::ComputeWrite:
      stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      accessMask = VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case BufferAccess::ComputeReadWrite:
      stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      break;
    case BufferAccess::VertexShaderRead:
      stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      accessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case BufferAccess::FragmentShaderRead:
      stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      accessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case BufferAccess::VertexInput:
      stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      accessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      break;
    case BufferAccess::IndexInput:
      stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      accessMask = VK_ACCESS_INDEX_READ_BIT;
      break;
    case BufferAccess::IndirectCommand:
      stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
      accessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
      break;
    default:
      throw std::runtime_error(
          "attempting to barrier on an unsupported buffer access "
          "configuration");
  }
}

/**
 * Sets a pipeline barrier for synchronization purposes.
 *
 * Makes the writes described by oldAccess available and visible to the reads
 * or writes described by newAccess, for the given range of the buffer. Use
 * this between two dispatches where the second reads what the first wrote --
 * recording them into the same command buffer does not order them on its own.
 *
 * @param commandBuffer The command buffer to record the barrier into
 * @param buffer The buffer to synchronize
 * @param oldAccess How the buffer was accessed before the barrier
 * @param newAccess How the buffer will be accessed after the barrier
 * @param offset (Optional) Byte offset from beginning
 * @param size (Optional) Size of the range to synchronize. Pass VK_WHOLE_SIZE
 * for the complete buffer range.
 *
 * @note - function was generated with AI - did not have time to finish this one
 * by hand
 */
void EngineBuffer::SetBufferPipelineBarrier(VkCommandBuffer commandBuffer,
                                            VkBuffer buffer,
                                            BufferAccess oldAccess,
                                            BufferAccess newAccess,
                                            VkDeviceSize offset,
                                            VkDeviceSize size)
{
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = 0;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = buffer;
  barrier.offset = offset;
  barrier.size = size;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;

  GetAccessInfo(oldAccess, true, srcStage, barrier.srcAccessMask);
  GetAccessInfo(newAccess, false, dstStage, barrier.dstAccessMask);

  vkCmdPipelineBarrier(commandBuffer,
                       srcStage,
                       dstStage,
                       0,
                       0,
                       nullptr,
                       1,
                       &barrier,
                       0,
                       nullptr);
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the
 * specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass
 * VK_WHOLE_SIZE to map the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 */
void EngineBuffer::map(VkDeviceSize size, VkDeviceSize offset)
{
  assert(buffer && allocation_->GetMemory() &&
         "Called map on buffer before create");
  vmaMapMemory(lveDevice.getAllocator(), allocation_, &mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void EngineBuffer::unmap()
{
  if (mapped) {
    vmaUnmapMemory(lveDevice.getAllocator(), allocation_);
    mapped = nullptr;
  }
}

/**
 * Copies the specified data to the mapped buffer. Default value writes
 * whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to
 * flush the complete buffer range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void EngineBuffer::writeToBuffer(void* data,
                                 VkDeviceSize size,
                                 VkDeviceSize offset)
{
  assert(mapped && "Cannot copy to unmapped buffer");

  if (size == VK_WHOLE_SIZE) {
    memcpy(mapped, data, bufferSize);
  }
  else {
    char* memOffset = (char*)mapped;
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass
 * VK_WHOLE_SIZE to flush the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult EngineBuffer::flush(VkDeviceSize size, VkDeviceSize offset)
{
  return vmaFlushAllocation(lveDevice.getAllocator(),
                            allocation_,
                            offset,
                            size);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass
 * VK_WHOLE_SIZE to invalidate the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult EngineBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
{
  return vmaInvalidateAllocation(lveDevice.getAllocator(),
                                 allocation_,
                                 offset,
                                 size);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo EngineBuffer::descriptorInfo(VkDeviceSize size,
                                                    VkDeviceSize offset)
{
  return VkDescriptorBufferInfo{
      buffer,
      offset,
      size,
  };
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of
 * index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void EngineBuffer::writeToIndex(void* data, int index)
{
  writeToBuffer(data, instanceSize, index * alignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make
 * it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult EngineBuffer::flushIndex(int index)
{
  return flush(alignmentSize, index * alignmentSize);
}

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo EngineBuffer::descriptorInfoForIndex(int index)
{
  return descriptorInfo(alignmentSize, index * alignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult EngineBuffer::invalidateIndex(int index)
{
  return invalidate(alignmentSize, index * alignmentSize);
}

}  // namespace GameEngine
