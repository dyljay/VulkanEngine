#pragma once

#include "engine_camera.hpp"
#include "engine_game_object.hpp"

namespace GameEngine {

#define MAX_LIGHTS 10

struct PointLight {
  glm::vec4 position{};
  glm::vec4 color{};
};

struct GlobalUbo {
  glm::mat4 projection{1.f};
  glm::mat4 view{1.f};
  glm::mat4 invView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.2f};
  PointLight pointLights[MAX_LIGHTS];
  int numActiveLights;
};

struct FrameInfo {
  int frameIndex;
  float frameTime;
  VkCommandBuffer commandBuffer;
  EngineCamera &camera;
  GameObject::Map &gameObjects;
};
} // namespace GameEngine
