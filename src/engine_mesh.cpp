#include "src/engine_mesh.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <vector>

#include "lib/utils.hpp"
#include "src/engine_buffer.hpp"
#include "src/engine_device.hpp"
#include "vulkan/vulkan_core.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <cassert>
#include <cstring>
#include <glm/gtx/hash.hpp>

namespace std {
template <>

struct hash<GameEngine::EngineMesh::Vertex> {
  size_t operator()(GameEngine::EngineMesh::Vertex const& vertex) const
  {
    size_t seed = 0;

    GameEngine::hashCombine(seed,
                            vertex.position,
                            vertex.color,
                            vertex.normal,
                            vertex.uv_x,
                            vertex.uv_y);

    return seed;
  }
};
}  // namespace std

namespace GameEngine {

EngineMesh::EngineMesh(EngineDevice& geDevice,
                       const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices,
                       const std::vector<GeoSurface>& surfaces)
    : surfaces_{surfaces}
{
  createBuffer(geDevice, indices, vertices);

  totalIndexCount();
}

EngineMesh::~EngineMesh() {}

/*
void EngineMesh::createVertexBuffers(const std::vector<Vertex> &vertices) {
  vertexCount = static_cast<uint32_t>(vertices.size());
  assert(vertexCount >= 3 && "Vertex must include at least 3 vertices");
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
  uint32_t vertexSize = sizeof(vertices[0]);

  EngineBuffer stagingBuffer{
      geDevice,
      vertexSize,
      vertexCount,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY,
  };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffer = std::make_unique<EngineBuffer>(
      geDevice, vertexSize, vertexCount,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo{};
  deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = vertexBuffer->getBuffer();
  vertexBufferAddress =
      vkGetBufferDeviceAddress(geDevice.device(), &deviceAddressInfo);

  geDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(),
                      bufferSize);
}

void EngineMesh::createIndexBuffers(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer) {
    return;
  }

  VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
  uint32_t indexSize = sizeof(indices[0]);

  EngineBuffer stagingBuffer{
      geDevice,
      indexSize,
      indexCount,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY,
  };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffer = std::make_unique<EngineBuffer>(
      geDevice, indexSize, indexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  geDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(),
                      bufferSize);
}
*/

void EngineMesh::totalIndexCount()
{
  for (auto& surface : surfaces_) {
    indexCount += surface.count;
  }
}

void EngineMesh::createBuffer(EngineDevice& geDevice,
                              std::vector<uint32_t> indices,
                              std::vector<Vertex> vertices)
{
  const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);

  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  hasIndexBuffer = indexBufferSize > 0;

  vertexBuffer = std::make_unique<EngineBuffer>(
      geDevice,
      vertexBufferSize,
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo{};
  deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = vertexBuffer->getBuffer();
  vertexBufferAddress =
      vkGetBufferDeviceAddress(geDevice.device(), &deviceAddressInfo);

  indexBuffer = std::make_unique<EngineBuffer>(
      geDevice,
      indexBufferSize,
      1,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  EngineBuffer stagingBuffer{
      geDevice,
      vertexBufferSize + indexBufferSize,
      1,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY,
  };

  stagingBuffer.map();  // = stagingBuffer.getAllocation()->GetMappedData();
  stagingBuffer.writeToBuffer((void*)vertices.data(), vertexBufferSize);
  stagingBuffer.writeToBuffer((void*)indices.data(),
                              indexBufferSize,
                              vertexBufferSize);

  geDevice.copyBuffer(stagingBuffer.getBuffer(),
                      vertexBuffer->getBuffer(),
                      vertexBufferSize);
  geDevice.copyBuffer(stagingBuffer.getBuffer(),
                      indexBuffer->getBuffer(),
                      indexBufferSize,
                      vertexBufferSize);
}

void EngineMesh::draw(VkCommandBuffer commandBuffer)
{
  vkCmdDrawIndexed(commandBuffer, indexCount, surfaces_.size(), 0, 0, 0);
}

void EngineMesh::bind(VkCommandBuffer commandBuffer)
{
  // VkBuffer buffers[] = {vertexBuffer->getBuffer()};
  // VkDeviceSize offsets[] = {0};
  //  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  if (hasIndexBuffer) {
    vkCmdBindIndexBuffer(commandBuffer,
                         indexBuffer->getBuffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);
  }
}
/*
std::vector<VkVertexInputBindingDescription>
EngineMesh::Vertex::getBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
EngineMesh::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

  // {location, binding, format, offset}
  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
  attributeDescriptions.push_back(
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
  attributeDescriptions.push_back(
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

  return attributeDescriptions;
}
*/
/*
std::unique_ptr<EngineMesh>
EngineMesh::createModelFromFile(EngineDevice &device,
                                const std::string &filePath) {
  Builder builder{};
  builder.loadModel(filePath);

  return std::make_unique<EngineMesh>(device, builder);
}

void EngineMesh::Builder::loadModel(const std::string &filePath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  std::string mtl_basePath = "./models/";

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        filePath.c_str(), mtl_basePath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  vertices.clear();
  indices.clear();

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      if (index.vertex_index >= 0) {
        vertex.position = {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2],
        };

        vertex.color = {
            attrib.colors[3 * index.vertex_index + 0],
            attrib.colors[3 * index.vertex_index + 1],
            attrib.colors[3 * index.vertex_index + 2],
        };
      }

      if (index.normal_index >= 0) {
        vertex.normal = {
            attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
        };
      }

      if (index.texcoord_index >= 0) {
        vertex.uv = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            attrib.texcoords[2 * index.texcoord_index + 1],
        };
      }

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }
}
*/
}  // namespace GameEngine
