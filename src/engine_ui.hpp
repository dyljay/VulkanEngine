#pragma once

#include "engine_descriptor.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdio>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <memory>
#include <stdlib.h>

namespace GameEngine {

static void check_vk_result(VkResult err) {
  if (err == 0)
    return;

  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);

  if (err < 0)
    abort();
}

class EngineUI {

public:
  EngineUI(EngineRenderer &geRenderer);
  ~EngineUI();

  void initImGUIPool();

  void initUI();

  void newFrame();

  void renderLightUI(glm::vec3 &color, float &intensity);

  void renderModelUI(TransformComponent &transform);

  void render(VkCommandBuffer commandBuffer);

  void endFrame();

private:
  EngineRenderer &geRenderer;

  std::unique_ptr<EngineDescriptorPool> imguiPool;
};

} // namespace GameEngine
