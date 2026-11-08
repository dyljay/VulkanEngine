#pragma once

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdlib.h>

#include <cstdio>
#include <glm/glm.hpp>
#include <memory>

#include "engine_descriptor.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

static void check_vk_result(VkResult err)
{
  if (err == 0) return;

  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);

  if (err < 0) abort();
}

class EngineUI {
 public:
  EngineUI(EngineRenderer& geRenderer);
  ~EngineUI();

  void setStyle();

  void newFrame();

  void beginSideBar(float width);
  void endSideBar();
  void renderLightUI(glm::vec3& color, float& intensity);

  void renderModelUI(TransformComponent& transform,
                     const std::string& name,
                     unsigned int id);

  void bvhUI(bool& showBVH);

  ImGuiIO& getIO();
  void render(VkCommandBuffer commandBuffer);

  void endFrame();

 private:
  void initImGUIPool();
  void initUI();

  EngineRenderer& geRenderer;

  std::unique_ptr<EngineDescriptorPool> imguiPool;
};

}  // namespace GameEngine
