#include "engine_model.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "glm/fwd.hpp"
#include "src/engine_device.hpp"
#include "src/engine_mesh.hpp"
#include "src/pbrMaterials.hpp"
#include "vulkan/vulkan_core.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <variant>
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

  constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                               fastgltf::Options::AllowDouble |
                               fastgltf::Options::LoadExternalBuffers |
                               fastgltf::Options::LoadExternalImages;
  fastgltf::Parser parser{};

  auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);

  if (load) {
    gltf = std::move(load.get());
  } else {
    throw std::runtime_error("failed to load model.");
  }

  // building descriptorPool
  buildDescriptorPool(MAX_SETS);

  // loadTextures
  loadTextures();

  // materials (need to be after textures to reference them in loadMaterials)
  loadMaterials();

  // then vertices
  loadVertices();
}

void EngineModel::loadVertices() {
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
              vertex.color = glm::vec4{1.f};
              vertex.uv_x = 0;
              vertex.uv_y = 0;
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
              vertices[initialVertex + index].uv_x = v.x;
              vertices[initialVertex + index].uv_y = v.y;
            });
      }

      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            gltf, gltf.accessors[(*colors).accessorIndex],
            [&](glm::vec4 v, size_t index) {
              vertices[initialVertex + index].color = v;
            });
      }

      if (p.materialIndex.has_value()) {
        newSurface.materialIndex = p.materialIndex.value();
      }

      surfaces.push_back(newSurface);
    }
    meshes.emplace_back(
        std::make_shared<EngineMesh>(geDevice, vertices, indices, surfaces));
  }
}

void EngineModel::buildDescriptorPool(uint32_t numSets) {
  std::vector<PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

  growablePool = EngineDescriptorPoolGrowable::Builder(geDevice)
                     .setNumSets(numSets)
                     .addPoolSizeRatioVector(sizes)
                     .build();
}

void EngineModel::loadTextures() {
  size_t numTextures = gltf.images.size();
  images.resize(numTextures);

  for (uint imageIndex = 0; imageIndex < numTextures; imageIndex++) {

    fastgltf::Image &gltfImage = gltf.images[imageIndex];
    auto texture = EngineTexture::createTexture(geDevice);

    std::visit(
        fastgltf::visitor{
            [&](fastgltf::sources::URI &filePath) {
              const std::string imageFilePath(filePath.uri.path().begin(),
                                              filePath.uri.path().end());

              int width = 0, height = 0, nrChannels = 0;

              stbi_uc *pixels = stbi_load(imageFilePath.c_str(), &width,
                                          &height, &nrChannels, STBI_rgb_alpha);

              VkFilter minFilter = getMinFilter();
              VkFilter magFilter = getMagFilter();
              VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
              texture->Init(width, height, imageFormat, pixels, minFilter,
                            magFilter);

              stbi_image_free(pixels);
            },
            [&](fastgltf::sources::Array &vector) {
              int width = 0, height = 0, nrChannels = 0;

              stbi_uc *pixels = stbi_load_from_memory(
                  reinterpret_cast<const stbi_uc *>(vector.bytes.data()),
                  static_cast<int>(vector.bytes.size()), &width, &height,
                  &nrChannels, STBI_rgb_alpha);

              VkFilter minFilter = getMinFilter();
              VkFilter magFilter = getMagFilter();
              VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
              texture->Init(width, height, imageFormat, pixels, minFilter,
                            magFilter);

              stbi_image_free(pixels);
            },
            [&](fastgltf::sources::BufferView &view) {
              auto &bufferView = gltf.bufferViews[view.bufferViewIndex];
              auto &bufferFromBufferView = gltf.buffers[bufferView.bufferIndex];

              std::visit(
                  fastgltf::visitor{
                      [&](auto &arg) {
                        throw std::runtime_error(
                            "not supported default branch");
                      },
                      [&](fastgltf::sources::Array &vector) {
                        int width = 0, height = 0, nrChannels = 0;

                        stbi_uc *pixels = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc *>(
                                vector.bytes.data() + bufferView.byteOffset),
                            static_cast<int>(bufferView.byteLength), &width,
                            &height, &nrChannels, STBI_rgb_alpha);

                        VkFilter minFilter = getMinFilter();
                        VkFilter magFilter = getMagFilter();
                        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
                        texture->Init(width, height, imageFormat, pixels,
                                      minFilter, magFilter);

                        stbi_image_free(pixels);
                      }},
                  bufferFromBufferView.data);
            },
            [&](auto &arg) { // default case for image data not supported
              throw std::runtime_error("Image format not supported");
            },
        },
        gltfImage.data);
    images[imageIndex] = std::move(texture);
  }
}

void EngineModel::loadMaterials() {
  size_t numMaterials = gltf.materials.size();
  materials.resize(numMaterials);

  PBRMaterial::increment_total_material(numMaterials);

  for (int matIndex = 0; matIndex < numMaterials; matIndex++) {
    auto material = std::make_shared<PBRMaterial>();
    fastgltf::Material &gltfMaterial = gltf.materials[matIndex];

    {
      material->properties.baseColor =
          glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data());
    }

    if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
      material->properties.baseColorMap =
          gltfMaterial.pbrData.baseColorTexture.value().textureIndex;
      material->properties.numFeatures |= PBRMaterial::GLSL_HAS_BASE_MAP;
    }

    if (gltfMaterial.normalTexture.has_value()) {
      material->properties.normalMap =
          gltfMaterial.normalTexture.value().textureIndex;
      material->properties.numFeatures |= PBRMaterial::GLSL_HAS_NORMAL_MAP;
    }

    // metallic/roughness values
    {
      material->properties.roughness = gltfMaterial.pbrData.roughnessFactor;
      material->properties.metallic = gltfMaterial.pbrData.metallicFactor;
    }

    if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) {
      material->properties.metallicRoughness =
          gltfMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
      material->properties.numFeatures |= PBRMaterial::GLSL_HAS_METAL_MAP;
    }

    // emissive color and strength
    // TODO: add emissive color/strength textures for lights in future
    {
    }

    material->properties.offset = 1;
    /*
    {
      auto &buffer = material->getMaterialBuffer();
      buffer = std::make_shared<EngineBuffer>(
          geDevice, sizeof(PBRMaterial), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VMA_MEMORY_USAGE_CPU_TO_GPU);
      buffer.get()->map();
      buffer.get()->writeToBuffer(&material->properties);
      buffer.get()->flush();
      buffer.get()->unmap();
    }
    */

    materials[matIndex] = std::move(material);
  }
}

VkFilter EngineModel::getMinFilter() { return VK_FILTER_NEAREST; }
VkFilter EngineModel::getMagFilter() { return VK_FILTER_NEAREST; }

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
