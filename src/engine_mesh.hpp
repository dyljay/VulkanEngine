#pragma once

#include "engine_buffer.hpp"
#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <algorithm>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <span>

namespace GameEngine {

struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
};

class EngineMesh {

public:
  struct Vertex {
    glm::vec3 position{};
    float uv_x;
    glm::vec3 normal{};
    float uv_y;
    glm::vec4 color{};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex &other) const {
      return position == other.position && color == other.color &&
             normal == other.normal && uv_x == other.uv_x && uv_y == other.uv_y;
    }
  };

  EngineMesh(EngineDevice &geDevice, const std::vector<Vertex> &vertices,
             const std::vector<uint32_t> &indices,
             const std::vector<GeoSurface> &surfaces);

  ~EngineMesh();

  EngineMesh(const EngineMesh &) = delete;
  EngineMesh &operator=(const EngineMesh &) = delete;

  static std::unique_ptr<EngineMesh>
  createModelFromFile(EngineDevice &device, const std::string &filePath);

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

  VkDeviceAddress getVertexBufferAddress() const { return vertexBufferAddress; }

private:
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  void createBuffer(EngineDevice &geDevice, std::vector<uint32_t> indices,
                    std::vector<Vertex> vertices);

  std::vector<GeoSurface> surfaces_;

  std::unique_ptr<EngineBuffer> vertexBuffer;
  uint32_t vertexCount;
  VkDeviceAddress vertexBufferAddress;

  bool hasIndexBuffer = false;
  std::unique_ptr<EngineBuffer> indexBuffer;
  uint32_t indexCount;
};

} // namespace GameEngine
