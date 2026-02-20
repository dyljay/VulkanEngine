#include "engine_mesh.hpp"
#include "lib/utils.hpp"
#include "src/engine_device.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
template <>

struct hash<GameEngine::EngineMesh::Vertex> {
  size_t operator()(GameEngine::EngineMesh::Vertex const &vertex) const {
    size_t seed = 0;

    GameEngine::hashCombine(seed, vertex.position, vertex.color, vertex.normal,
                            vertex.uv);

    return seed;
  }
};
} // namespace std

namespace GameEngine {

EngineMesh::EngineMesh(EngineDevice &geDevice, const Builder &builder)
    : geDevice{geDevice} {
  createVertexBuffers(builder.vertices);
  createIndexBuffers(builder.indices);
}

EngineMesh::~EngineMesh() {}

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
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffer = std::make_unique<EngineBuffer>(
      geDevice, vertexSize, vertexCount,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  };

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffer = std::make_unique<EngineBuffer>(
      geDevice, indexSize, indexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  geDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(),
                      bufferSize);
}

void EngineMesh::draw(VkCommandBuffer commandBuffer) {
  if (hasIndexBuffer) {
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
  } else {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }
}

void EngineMesh::bind(VkCommandBuffer commandBuffer) {
  VkBuffer buffers[] = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  if (hasIndexBuffer) {
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
  }
}

std::unique_ptr<EngineMesh>
EngineMesh::createMesh(EngineDevice &geDevice, const tinyobj::attrib_t &attrib,
                       const tinyobj::shape_t &shape) {
  Builder builder{};
  builder.loadMesh(attrib, shape);

  return std::make_unique<EngineMesh>(geDevice, builder);
}

void EngineMesh::Builder::loadMesh(const tinyobj::attrib_t &attrib,
                                   const tinyobj::shape_t &shape) {

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  vertices.clear();
  indices.clear();

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
} // namespace GameEngine
