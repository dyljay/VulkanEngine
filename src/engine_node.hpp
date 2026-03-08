#pragma once

#include "engine_mesh.hpp"
#include "glm/fwd.hpp"
#include <memory>
#include <vector>

namespace GameEngine {

struct DrawContext;

class IRenderable {
  virtual void Draw(const glm::mat4 transform, DrawContext &ctx) = 0;
};

class Node : public IRenderable {
public:
  std::unique_ptr<Node> parent;
  std::vector<std::unique_ptr<Node>> children;

  Node();
  ~Node();

  void refreshTransform(const glm::mat4 &parentMatrix);

  virtual void Draw(const glm::mat4 &transform, DrawContext &ctx);

private:
  glm::mat4 localTransform;
  glm::mat4 worldTransform;
};

class MeshNode : public Node {
public:
  virtual void Draw(const glm::mat4 transform, DrawContext &ctx) override;

private:
  std::unique_ptr<EngineMesh> mesh;
};
} // namespace GameEngine
