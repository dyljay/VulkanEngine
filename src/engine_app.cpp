#include "engine_app.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "engine_buffer.hpp"
#include "engine_camera.hpp"
#include "engine_keyboardmovement.hpp"
#include "engine_texture.hpp"
#include "engine_ui.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "point_light_system.hpp"
#include "simple_render_system.hpp"
#include "src/engine_descriptor.hpp"
#include "src/engine_device.hpp"
#include "src/engine_game_object.hpp"
#include "src/engine_model.hpp"
#include "src/engine_ui.hpp"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <ios>
#include <memory>

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

  bool userSeeMouse = false;

  float red = 0.8f;
  float green = 0.2f;
  float blue = 0.7f;
  float intensity = 1.0f;

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
        if (e.key.key == SDLK_F) {
          if (!userSeeMouse) {
            userSeeMouse = true;
            SDL_SetWindowRelativeMouseMode(geWindow.getSDLWindow(), false);
          } else {
            userSeeMouse = false;
            SDL_SetWindowRelativeMouseMode(geWindow.getSDLWindow(), true);
          }
        }
      }

      if (e.type == SDL_EVENT_MOUSE_MOTION && !userSeeMouse) {
        cameraController.handleMouseMovements(e, frameTime, viewerObject);
      }

      if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        geWindow.setWindowResizedFlag();
      }

      ImGui_ImplSDL3_ProcessEvent(&e);
    }

    // TODOEND

    if (!userSeeMouse) {
      cameraController.moveInXYZPlane(frameTime, viewerObject);
      camera.setViewYXZ(viewerObject.transform.translation,
                        viewerObject.transform.rotation);
    }

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

      // ui
      // TODO: make this a bit cleaner - look into if there are better methods
      // to doing the ui instead of all here in the loop
      if (userSeeMouse) {
        ui.newFrame();
        if (ImGui::Begin("Light Color")) {
          ImGui::Text("RGB");
          ImGui::SliderFloat("Red", &red, 0.0f, 1.0f);
          ImGui::SliderFloat("Green", &green, 0.0f, 1.0f);
          ImGui::SliderFloat("Blue", &blue, 0.0f, 1.0f);
          ImGui::Text("Intensity");
          ImGui::SliderFloat("Intensity", &intensity, 0.f, 10.f);
        }
        ImGui::End();
        ui.render(commandBuffer);
        ui.endFrame();

        for (auto &kv : geObjects) {
          auto &obj = kv.second;
          if (obj.pointLight == nullptr)
            continue;

          obj.color = {red, green, blue};
          obj.pointLight->lightIntensity = intensity;
        }
      }

      // ui end

      geRenderer.endSwapChainRenderPass(commandBuffer);
      geRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(geDevice.device());
}

void EngineApp::loadGameObjects() {

  std::shared_ptr<EngineModel> geModel = EngineModel::createModelFromFile(
      geDevice, "./models/cyberpunk_woman.glb");
  auto flatVase = GameObject::createGameObject();
  flatVase.model = geModel;
  flatVase.transform.translation = {0.35f, -.7f, -.4f};
  flatVase.transform.rotation = {glm::radians(90.f), glm::radians(180.f), 0.0f};
  geObjects.emplace(flatVase.getID(), std::move(flatVase));

  std::vector<glm::vec3> lightColors = {{0.8f, 0.2f, .7f}};

  for (int i = 0; i < lightColors.size(); i++) {
    auto pointLight = GameObject::makePointLight(12.f);
    pointLight.color = lightColors[i];

    auto rotateLight = glm::rotate(
        glm::mat4(1.0f), (i * glm::two_pi<float>()) / lightColors.size(),
        {0.f, 1.f, 0.f});

    pointLight.transform.translation =
        glm::vec3(rotateLight * glm::vec4(-1.f, 1.f, 1.f, 1.f));

    geObjects.emplace(pointLight.getID(), std::move(pointLight));
  }
}

} // namespace GameEngine
