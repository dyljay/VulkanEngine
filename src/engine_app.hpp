#pragma once

#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "engine_window.hpp"

// std
#include <memory>

namespace GameEngine {
class EngineApp {
public:
  static constexpr int MAX_DESCRIPTOR_SET = 1;

  void run();
  void updateGameObjects(float deltaTime);
  static constexpr int WIDTH = 1080;
  static constexpr int HEIGHT = 780;

  EngineApp();
  ~EngineApp();

  EngineApp(const EngineApp &) = delete;
  EngineApp &operator=(const EngineApp &) = delete;

private:
  void loadGameObjects();

  EngineWindow geWindow{WIDTH, HEIGHT, "Skumpwit"};
  EngineDevice geDevice{geWindow};
  EngineRenderer geRenderer{geWindow, geDevice};

  std::unique_ptr<EngineDescriptorPoolGrowable> globalPool{};
  GameObject::Map geObjects;

  const std::array<std::string, 6> cubeTextureFilePaths = {
      "./textures/px.jpg", "./textures/nx.jpg", "./textures/py.jpg",
      "./textures/ny.jpg", "./textures/pz.jpg", "./textures/nz.jpg",
  };
};
} // namespace GameEngine
