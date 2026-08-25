#pragma once

#include <memory>
#include <vector>

#include "engine_mesh.hpp"
#include "glm/fwd.hpp"

namespace GameEngine {

class IRenderable {
  virtual void Draw(const glm::mat4& transform) = 0;
};

class Node : public IRenderable {
 public:
  std::weak_ptr<Node> parent;
  std::vector<std::shared_ptr<Node>> children;

  Node();
  ~Node();

  void refreshTransform(const glm::mat4& parentMatrix);

  virtual void Draw(const glm::mat4& transform);

  glm::mat4 localTransform;
  glm::mat4 worldTransform;
};

class MeshNode : public Node {
 public:
  MeshNode(std::shared_ptr<EngineMesh> mesh);
  ~MeshNode();
  virtual void Draw(const glm::mat4& transform) override;

 private:
  std::shared_ptr<EngineMesh> mesh;
};
}  // namespace GameEngine
