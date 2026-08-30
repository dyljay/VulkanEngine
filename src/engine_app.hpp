#pragma once

#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "engine_ui.hpp"
#include "engine_window.hpp"
#include "shaderList.hpp"
// std
#include <stdbool.h>

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace GameEngine {

class EngineApp {
 public:
  static constexpr int MAX_DESCRIPTOR_SET = 100;

  void run();

  static constexpr int WIDTH = 1180;
  static constexpr int HEIGHT = 980;

  EngineApp();
  ~EngineApp();

  EngineApp(const EngineApp&) = delete;
  EngineApp& operator=(const EngineApp&) = delete;

 private:
  void loadGameObjects(EngineDescriptorPoolGrowable& growablePool);

  void populateDescriptorSetLayouts(DescriptorSetLayouts& descriptorSetLayouts);

  void populateMatTexDescriptorSets(DescriptorSets& descriptorSets,
                                    DescriptorSetLayouts& descriptorSetLayouts);

  void renderUI(EngineUI& ui);
  void updateScene();

  EngineWindow geWindow{WIDTH, HEIGHT, "WIT Engine"};
  EngineDevice geDevice{geWindow};
  EngineRenderer geRenderer{geWindow, geDevice};

  OffScreenRenderer offscreenRenderer{geWindow, geDevice};

  std::unique_ptr<EngineDescriptorPoolGrowable> globalPool{};
  GameObject::Map geObjects;

  const std::array<std::string, 6> cubeTextureFilePaths = {
      "./images/px.jpg",
      "./images/nx.jpg",
      "./images/py.jpg",
      "./images/ny.jpg",
      "./images/pz.jpg",
      "./images/nz.jpg",
  };

  bool hasClicked = false;
  bool showbbox = false;
  std::unordered_map<uint32_t, bool> selectedObjects;
};
}  // namespace GameEngine
