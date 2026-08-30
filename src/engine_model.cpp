#include "engine_model.hpp"

#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "engine_descriptor.hpp"
#include "engine_node.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "glm/fwd.hpp"
#include "src/engine_device.hpp"
#include "src/engine_mesh.hpp"
#include "src/pbrMaterials.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

EngineModel::EngineModel(EngineDevice& geDevice,
                         const std::filesystem::path& path,
                         const std::string modelName,
                         EngineDescriptorPoolGrowable& growablePool)
    : geDevice{geDevice},
      modelName{modelName},
      growablePool{growablePool}
{
  loadModel(path);
}

EngineModel::~EngineModel() {}

std::unique_ptr<EngineModel> EngineModel::createModelFromFile(
    EngineDevice& geDevice,
    const std::filesystem::path& filePath,
    const std::string modelName,
    EngineDescriptorPoolGrowable& growablePool)
{
  return std::make_unique<EngineModel>(geDevice,
                                       filePath,
                                       modelName,
                                       growablePool);
}

void EngineModel::loadModel(const std::filesystem::path& filePath)
{
  auto data = fastgltf::GltfDataBuffer::FromPath(filePath);

  constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                               fastgltf::Options::AllowDouble |
                               fastgltf::Options::LoadExternalBuffers |
                               fastgltf::Options::LoadExternalImages;
  fastgltf::Parser parser{};

  auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);
  fastgltf::Asset gltf;

  if (load) {
    gltf = std::move(load.get());
  }
  else {
    throw std::runtime_error("failed to load model.");
  }

  // loadTextures
  loadTextures(gltf);

  // materials
  loadMaterials(gltf);

  // then vertices
  loadVertices(gltf);

  // finally nodes
  loadNodes(gltf);
}

void EngineModel::loadNodes(fastgltf::Asset& gltf)
{
  // load all nodes and their meshes
  for (fastgltf::Node& node : gltf.nodes) {
    std::shared_ptr<Node> newNode =
        (node.meshIndex.has_value())
            ? std::make_shared<MeshNode>(meshes[*node.meshIndex])
            : std::make_shared<Node>();

    std::visit(
        fastgltf::visitor{
            [&](fastgltf::math::fmat4x4& matrix) {
              memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
            },
            [&](fastgltf::TRS& trs) {
              glm::vec3 tl(trs.translation[0],
                           trs.translation[1],
                           trs.translation[2]);
              glm::quat rot(trs.rotation[3],
                            trs.rotation[0],
                            trs.rotation[1],
                            trs.rotation[2]);
              glm::vec3 sc(trs.scale[0], trs.scale[1], trs.scale[2]);

              glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
              glm::mat4 rm = glm::toMat4(rot);
              glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

              newNode->localTransform = tm * rm * sm;
            }},
        node.transform);

    nodes.push_back(std::move(newNode));
  }
  for (int i = 0; i < gltf.nodes.size(); i++) {
    fastgltf::Node& node = gltf.nodes[i];
    std::shared_ptr<Node>& sceneNode = nodes[i];

    for (auto& c : node.children) {
      sceneNode->children.push_back(nodes[c]);
      nodes[c]->parent = sceneNode;
    }
  }

  // find the top nodes, with no parents
  for (auto& node : nodes) {
    if (node->parent.lock() == nullptr) {
      topNodes.push_back(node);
      node->refreshTransform(glm::mat4{1.f});
    }
  }
}

void EngineModel::loadVertices(fastgltf::Asset& gltf)
{
  std::vector<uint32_t> indices;
  std::vector<EngineMesh::Vertex> vertices;
  std::vector<GeoSurface> surfaces;

  for (fastgltf::Mesh& mesh : gltf.meshes) {
    indices.clear();
    vertices.clear();
    surfaces.clear();
    AABB bbox;

    for (auto&& p : mesh.primitives) {
      GeoSurface newSurface;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count =
          (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initialVertex = vertices.size();

      {
        fastgltf::Accessor& indexAccessor =
            gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexAccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
            gltf,
            indexAccessor,
            [&](std::uint32_t index) {
              indices.push_back(index + initialVertex);
            });
      }

      // FIXME: need to use min/max of accessor as it is much more eficient

      {
        fastgltf::Accessor& posAccessor =
            gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf,
            posAccessor,
            [&](glm::vec3 v, size_t index) {
              EngineMesh::Vertex vertex;
              vertex.position = v;
              vertex.normal = {1, 0, 0};
              vertex.color = glm::vec4{1.f};
              vertex.uv_x = 0;
              vertex.uv_y = 0;
              vertices[initialVertex + index] = vertex;
              bbox.testBounds(v);
            });
      }

      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf,
            gltf.accessors[(*normals).accessorIndex],
            [&](glm::vec3 v, size_t index) {
              vertices[initialVertex + index].normal = v;
            });
      }

      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            gltf,
            gltf.accessors[(*uv).accessorIndex],
            [&](glm::vec2 v, size_t index) {
              vertices[initialVertex + index].uv_x = v.x;
              vertices[initialVertex + index].uv_y = v.y;
            });
      }

      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            gltf,
            gltf.accessors[(*colors).accessorIndex],
            [&](glm::vec4 v, size_t index) {
              vertices[initialVertex + index].color = v;
            });
      }

      if (p.materialIndex.has_value()) {
        newSurface.materialIndex = p.materialIndex.value();
      }

      surfaces.push_back(newSurface);
    }
    meshes.emplace_back(std::make_shared<EngineMesh>(geDevice,
                                                     vertices,
                                                     indices,
                                                     surfaces,
                                                     bbox));
  }
}

void EngineModel::loadTextures(fastgltf::Asset& gltf)
{
  size_t numTextures = gltf.images.size();
  images.resize(numTextures);

  for (uint imageIndex = 0; imageIndex < numTextures; imageIndex++) {
    fastgltf::Image& gltfImage = gltf.images[imageIndex];
    auto texture = EngineTexture::createTexture(geDevice);

    std::visit(
        fastgltf::visitor{
            [&](fastgltf::sources::URI& filePath) {
              const std::string imageFilePath(filePath.uri.path().begin(),
                                              filePath.uri.path().end());

              int width = 0, height = 0, nrChannels = 0;

              stbi_uc* pixels = stbi_load(imageFilePath.c_str(),
                                          &width,
                                          &height,
                                          &nrChannels,
                                          STBI_rgb_alpha);

              VkFilter minFilter = VK_FILTER_LINEAR;
              VkFilter magFilter = VK_FILTER_LINEAR;
              VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
              VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
              texture->Init(width,
                            height,
                            imageFormat,
                            pixels,
                            minFilter,
                            magFilter,
                            mipMapMode);

              stbi_image_free(pixels);
            },
            [&](fastgltf::sources::Array& vector) {
              int width = 0, height = 0, nrChannels = 0;

              stbi_uc* pixels = stbi_load_from_memory(
                  reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                  static_cast<int>(vector.bytes.size()),
                  &width,
                  &height,
                  &nrChannels,
                  STBI_rgb_alpha);

              VkFilter minFilter = VK_FILTER_LINEAR;
              VkFilter magFilter = VK_FILTER_LINEAR;
              VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
              VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
              texture->Init(width,
                            height,
                            imageFormat,
                            pixels,
                            minFilter,
                            magFilter,
                            mipMapMode);

              stbi_image_free(pixels);
            },
            [&](fastgltf::sources::BufferView& view) {
              auto& bufferView = gltf.bufferViews[view.bufferViewIndex];
              auto& bufferFromBufferView = gltf.buffers[bufferView.bufferIndex];

              std::visit(
                  fastgltf::visitor{
                      [&](auto& arg) {
                        throw std::runtime_error(
                            "not supported default branch");
                      },
                      [&](fastgltf::sources::Array& vector) {
                        int width = 0, height = 0, nrChannels = 0;

                        stbi_uc* pixels = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc*>(
                                vector.bytes.data() + bufferView.byteOffset),
                            static_cast<int>(bufferView.byteLength),
                            &width,
                            &height,
                            &nrChannels,
                            STBI_rgb_alpha);

                        VkFilter minFilter = VK_FILTER_LINEAR;
                        VkFilter magFilter = VK_FILTER_LINEAR;
                        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
                        VkSamplerMipmapMode mipMapMode =
                            VK_SAMPLER_MIPMAP_MODE_LINEAR;
                        texture->Init(width,
                                      height,
                                      imageFormat,
                                      pixels,
                                      minFilter,
                                      magFilter,
                                      mipMapMode);

                        stbi_image_free(pixels);
                      }},
                  bufferFromBufferView.data);
            },
            [&](auto& arg) {  // default case for image data not
                              // supported
              throw std::runtime_error("Image format not supported");
            },
        },
        gltfImage.data);
    images[imageIndex] = std::move(texture);
  }
}

void EngineModel::loadMaterials(fastgltf::Asset& gltf)
{
  size_t numMaterials = gltf.materials.size();
  materials.resize(numMaterials);

  PBRMaterial::increment_total_material(numMaterials);

  for (int matIndex = 0; matIndex < numMaterials; matIndex++) {
    auto material = std::make_shared<PBRMaterial>();
    fastgltf::Material& gltfMaterial = gltf.materials[matIndex];

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

    materials[matIndex] = std::move(material);
  }
}

VkFilter EngineModel::getFilter(fastgltf::Filter filter)
{
  switch (filter) {
    // nearest samplers
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
      return VK_FILTER_NEAREST;

    // linear samplers
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:

    default:
      return VK_FILTER_LINEAR;
  }
}

VkSamplerMipmapMode EngineModel::getSampler(fastgltf::Filter filter)
{
  switch (filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
      return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
      return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
}

void EngineModel::bind(VkCommandBuffer commandBuffer)
{
  for (auto& mesh : meshes) {
    mesh->bind(commandBuffer);
  }
}

void EngineModel::draw(VkCommandBuffer commandBuffer)
{
  for (auto& mesh : meshes) {
    mesh->draw(commandBuffer);
  }
}

}  // namespace GameEngine
