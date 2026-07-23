#pragma once

#include "engine_model.hpp"

#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <memory>

namespace GameEngine {

struct TransformComponent {
  glm::vec3 translation{};
  glm::vec3 scale{1.f, 1.f, 1.f};
  glm::vec3 rotation;

  glm::mat4 mat4();

  glm::mat3 normalMatrix();
};

struct PointLightComponent {
  float lightIntensity = 1.0f;
};

class GameObject {

public:
  using id_t = unsigned int;
  using Map = std::map<id_t, GameObject>;

  static GameObject createGameObject() {
    static id_t currentID = 1;
    return GameObject{currentID++};
  }

  static GameObject makePointLight(float intensity = 10.f, float radius = 0.1f,
                                   glm::vec3 color = glm::vec3(1.0f));

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = delete;

  id_t getID() { return id; }

  glm::vec3 color{};
  TransformComponent transform{};

  bool isSelected = false;

  std::shared_ptr<EngineModel> model{};
  std::unique_ptr<PointLightComponent> pointLight = nullptr;

private:
  GameObject(id_t objID) : id{objID} {}

  id_t id;
};
} // namespace GameEngine
