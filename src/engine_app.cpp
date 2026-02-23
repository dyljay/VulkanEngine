#include "engine_app.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "engine_buffer.hpp"
#include "engine_camera.hpp"
#include "engine_keyboardmovement.hpp"
#include "engine_texture.hpp"
#include "engine_ui.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "point_light_system.hpp"
#include "simple_render_system.hpp"
#include "src/engine_descriptor.hpp"
#include "src/engine_device.hpp"
#include "src/engine_game_object.hpp"
#include "src/engine_model.hpp"
#include "src/engine_ui.hpp"
#include "vulkan/vulkan_core.h"
#include <memory>

#include <iostream>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <chrono>

namespace GameEngine {

EngineApp::EngineApp() {
  globalPool = EngineDescriptorPool::Builder(geDevice)
                   .setMaxSets(MAX_DESCRIPTOR_SET)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();

  loadGameObjects();
}

EngineApp::~EngineApp() {}

void EngineApp::run() {
  std::vector<std::unique_ptr<EngineBuffer>> uboBuffers(
      EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] = std::make_unique<EngineBuffer>(
        geDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);
    uboBuffers[i]->map();
  }

  auto globalSetLayout = EngineDescriptorSetLayout::Builder(geDevice)
                             .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_ALL_GRAPHICS)
                             .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(
      EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < globalDescriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    EngineDescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  EngineUI ui{geRenderer};

  SimpleRenderSystem simpleRenderSystem{
      geDevice, geRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout()};

  PointLightSystem pointLightSystem{geDevice,
                                    geRenderer.getSwapChainRenderPass(),
                                    globalSetLayout->getDescriptorSetLayout()};

  EngineCamera camera{};
  camera.setViewDirection(glm::vec3(0.0f), glm::vec3(0.f, 0.f, 1.f));

  auto viewerObject = GameObject::createGameObject();
  viewerObject.transform.translation.z = -2.5f;
  EngineController cameraController{};

  auto currentTime = std::chrono::high_resolution_clock::now();

  bool shouldQuit = false;
  SDL_Event e;

  while (!shouldQuit) {
    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;

    // fps counter
    float fps = 1.f / frameTime;

    // TODO: might be worth moving this into it's own method. if you're gonna
    // process all events here, should be moved out, maybe to
    // KeyboardMovementController?

    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_EVENT_QUIT) {
        shouldQuit = true;
      }

      if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_ESCAPE) {
          shouldQuit = true;
        }
      }

      if (e.type == SDL_EVENT_MOUSE_MOTION) {
        cameraController.handleMouseMovements(e, frameTime, viewerObject);
      }

      if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        geWindow.setWindowResizedFlag();
      }

      ImGui_ImplSDL3_ProcessEvent(&e);
    }

    // TODOEND

    cameraController.moveInXYZPlane(frameTime, viewerObject);
    camera.setViewYXZ(viewerObject.transform.translation,
                      viewerObject.transform.rotation);

    float aspect = geRenderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(45.f), aspect, 0.1f, 1000.f);

    if (auto commandBuffer = geRenderer.beginFrame()) {
      int frameIndex = geRenderer.getFrameIndex();
      FrameInfo frameInfo{frameIndex,
                          frameTime,
                          commandBuffer,
                          camera,
                          globalDescriptorSets[frameIndex],
                          geObjects};

      // update objects
      GlobalUbo ubo{};
      ubo.projection = camera.getProjection();
      ubo.view = camera.getView();
      ubo.invView = camera.getInverseView();
      pointLightSystem.update(frameInfo, ubo);
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      // draw calls
      geRenderer.beginSwapChainRenderPass(commandBuffer);
      simpleRenderSystem.renderGameObjects(frameInfo);
      pointLightSystem.render(frameInfo);

      ui.newFrame();
      ui.render(commandBuffer);
      ui.endFrame();

      geRenderer.endSwapChainRenderPass(commandBuffer);
      geRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(geDevice.device());
}

void EngineApp::loadGameObjects() {
  /**
  std::shared_ptr<EngineModel> geModel =
      EngineModel::createModelFromFile(geDevice, "models/flat_vase.obj");
  auto flatVase = GameObject::createGameObject();
  flatVase.model = geModel;
  flatVase.transform.translation = {-.5f, .5f, 0.f};
  flatVase.transform.scale = {3.f, 1.5f, 3.f};
  geObjects.emplace(flatVase.getID(), std::move(flatVase));

  geModel =
      EngineModel::createModelFromFile(geDevice, "models/smooth_vase.obj");
  auto smoothVase = GameObject::createGameObject();
  smoothVase.model = geModel;
  smoothVase.transform.translation = {.5f, .5f, 0.f};
  smoothVase.transform.scale = {3.f, 1.5f, 3.f};
  geObjects.emplace(smoothVase.getID(), std::move(smoothVase));


  geModel = EngineModel::createModelFromFile(geDevice, "models/quad.obj");
  auto floor = GameObject::createGameObject();
  floor.model = geModel;
  floor.transform.translation = {0.f, .5f, 0.f};
  floor.transform.scale = {3.f, 1.f, 3.f};
  geObjects.emplace(floor.getID(), std::move(floor));
  */

  std::shared_ptr<EngineModel> geModel = EngineModel::createModelFromFile(
      geDevice, "models/Residential_Buildings_001.obj");
  auto building = GameObject::createGameObject();
  building.model = geModel;
  building.transform.translation = {0.f, 0.f, 15.f};
  building.transform.scale = {0.5f, 0.5f, 0.5f};
  building.transform.rotation = {0.f, 1.5f * glm::pi<float>(),
                                 glm::pi<float>()};
  geObjects.emplace(building.getID(), std::move(building));

  geModel = EngineModel::createModelFromFile(
      geDevice, "models/Residential_Buildings_002.obj");
  auto building_2 = GameObject::createGameObject();
  building_2.model = geModel;
  building_2.transform.translation = {15.f, 0.f, 0.f};
  building_2.transform.scale = {0.5f, 0.5f, 0.5f};
  building_2.transform.rotation = {0.0f, 0.f, glm::pi<float>()};
  geObjects.emplace(building_2.getID(), std::move(building_2));

  // std::vector<glm::vec3> lightColors{
  //     {1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f},
  //     {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f} //
  // };
  std::vector<glm::vec3> lightColors = {{1.f, 1.f, 1.f}};

  for (int i = 0; i < lightColors.size(); i++) {
    auto pointLight = GameObject::makePointLight(12.f);
    pointLight.color = lightColors[i];

    auto rotateLight = glm::rotate(
        glm::mat4(1.0f), (i * glm::two_pi<float>()) / lightColors.size(),
        {0.f, -1.f, 0.f});

    pointLight.transform.translation =
        glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));

    geObjects.emplace(pointLight.getID(), std::move(pointLight));
  }
}

} // namespace GameEngine
