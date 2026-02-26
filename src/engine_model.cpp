#include "engine_model.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "glm/fwd.hpp"
#include "src/engine_device.hpp"
#include "src/engine_mesh.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

namespace GameEngine {

EngineModel::EngineModel(EngineDevice &geDevice,
                         const std::filesystem::path &path)
    : geDevice{geDevice} {
  loadModel(path);
}

EngineModel::~EngineModel() {}

std::unique_ptr<EngineModel>
EngineModel::createModelFromFile(EngineDevice &geDevice,
                                 const std::filesystem::path &filePath) {

  return std::make_unique<EngineModel>(geDevice, filePath);
}

void EngineModel::loadModel(const std::filesystem::path &filePath) {
  auto data = fastgltf::GltfDataBuffer::FromPath(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);

  if (load) {
    gltf = std::move(load.get());
  } else {
    throw std::runtime_error("failed to load model.");
  }
  std::vector<uint32_t> indices;
  std::vector<EngineMesh::Vertex> vertices;
  std::vector<GeoSurface> surfaces;

  for (fastgltf::Mesh &mesh : gltf.meshes) {

    indices.clear();
    vertices.clear();
    surfaces.clear();

    for (auto &&p : mesh.primitives) {

      GeoSurface newSurface;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count =
          (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initialVertex = vertices.size();

      {
        fastgltf::Accessor &indexAccessor =
            gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexAccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
            gltf, indexAccessor, [&](std::uint32_t index) {
              indices.push_back(index + initialVertex);
            });
      }

      {
        fastgltf::Accessor &posAccessor =
            gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
              EngineMesh::Vertex vertex;
              vertex.position = v;
              vertex.normal = {1, 0, 0};
              vertex.color = glm::vec3{1.f};
              vertex.uv = {0, 0};
              vertices[initialVertex + index] = vertex;
            });
      }

      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, gltf.accessors[(*normals).accessorIndex],
            [&](glm::vec3 v, size_t index) {
              vertices[initialVertex + index].normal = v;
            });
      }

      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            gltf, gltf.accessors[(*uv).accessorIndex],
            [&](glm::vec2 v, size_t index) {
              vertices[initialVertex + index].uv = v;
            });
      }

      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, gltf.accessors[(*colors).accessorIndex],
            [&](glm::vec3 v, size_t index) {
              vertices[initialVertex + index].color = v;
            });
      }
      surfaces.push_back(newSurface);
    }
    meshes.emplace_back(
        std::make_shared<EngineMesh>(geDevice, vertices, indices, surfaces));
  }
}

void EngineModel::bind(VkCommandBuffer commandBuffer) {
  for (auto &mesh : meshes) {
    mesh->bind(commandBuffer);
  }
}

void EngineModel::draw(VkCommandBuffer commandBuffer) {
  for (auto &mesh : meshes) {
    mesh->draw(commandBuffer);
  }
}

} // namespace GameEngine
