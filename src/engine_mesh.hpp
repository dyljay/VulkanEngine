#pragma once

#include "engine_buffer.hpp"
#include "engine_device.hpp"
#include "engine_texture.hpp"
#include <memory>
#include <vector>

#include "lib/tiny_obj_loader.h"
#include "vulkan/vulkan_core.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

    void loadMesh(const tinyobj::attrib_t &attrib,
                  const tinyobj::shape_t &shape);
  };

  static std::unique_ptr<EngineMesh> createMesh(EngineDevice &geDevice,
                                                const tinyobj::attrib_t &attrub,
                                                const tinyobj::shape_t &shape);

  EngineMesh(EngineDevice &geDevice, const Builder &builder);
  ~EngineMesh();

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

  EngineMesh(const EngineMesh &) = delete;
  EngineMesh &operator=(const EngineMesh &) = delete;

private:
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);
  void createTextureImage(const std::string &filePath);

  EngineDevice &geDevice;

  std::unique_ptr<EngineBuffer> vertexBuffer;
  uint32_t vertexCount;

  bool hasIndexBuffer = false;
  std::unique_ptr<EngineBuffer> indexBuffer;
  uint32_t indexCount;

  std::unique_ptr<EngineTexture> texture;
};
} // namespace GameEngine
