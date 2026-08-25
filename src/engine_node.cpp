#include "engine_node.hpp"

#include <memory>

#include "engine_mesh.hpp"
#include "glm/fwd.hpp"

namespace GameEngine {

Node::Node() {}

Node::~Node() {}

void Node::Draw(const glm::mat4& transform)
{
  for (auto& c : children) {
    c->Draw(transform);
  }
}

void Node::refreshTransform(const glm::mat4& topMatrix)
{
  worldTransform = topMatrix * localTransform;

  for (auto& c : children) {
    c->refreshTransform(worldTransform);
  }
}

MeshNode::MeshNode(std::shared_ptr<EngineMesh> mesh)
    : mesh{mesh}
{}

MeshNode::~MeshNode() {}

void MeshNode::Draw(const glm::mat4& transform)
{
  glm::mat4 nodeMatrix = transform * worldTransform;
  mesh->getLocalMatrix() = nodeMatrix;

  Node::Draw(transform);
}
}  // namespace GameEngine
