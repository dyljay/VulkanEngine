#pragma once

#include "engine_buffer.hpp"
#include "engine_device.hpp"
#include "vulkan/vulkan_core.h"
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <span>

namespace GameEngine {

class EngineMesh {

public:
  struct Vertex {
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex &other) const {
      return position == other.position && color == other.color &&
             normal == other.normal && uv == other.uv;
    }
  };

  struct Builder {
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

    void loadModel(const std::string &filePath);
  };

  EngineMesh(EngineDevice &device, const EngineMesh::Builder &builder);
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
  void createBuffer(std::vector<uint32_t> indices,
                    std::vector<Vertex> vertices);

  EngineDevice &geDevice;

  std::unique_ptr<EngineBuffer> vertexBuffer;
  uint32_t vertexCount;
  VkDeviceAddress vertexBufferAddress;

  bool hasIndexBuffer = false;
  std::unique_ptr<EngineBuffer> indexBuffer;
  uint32_t indexCount;
};

} // namespace GameEngine
