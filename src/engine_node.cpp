#include "engine_node.hpp"

namespace GameEngine {

Node::Node() {}

Node::~Node() {}

void Node::Draw(const glm::mat4 &transform, DrawContext &ctx) {
  for (auto &c : children) {
    c->Draw(transform, ctx);
  }
}

void Node::refreshTransform(const glm::mat4 &topMatrix) {
  worldTransform = topMatrix * localTransform;

  for (auto &c : children) {
    c->refreshTransform(worldTransform);
  }
}

void MeshNode::Draw(const glm::mat4 transform, DrawContext &ctx) {}
} // namespace GameEngine
