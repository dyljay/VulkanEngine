#include "engine_app.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"
#include "bbox_system.hpp"
#include "bvh.hpp"
#include "cubemap_system.hpp"
#include "engine_buffer.hpp"
#include "engine_camera.hpp"
#include "engine_descriptor.hpp"
#include "engine_device.hpp"
#include "engine_frame_info.hpp"
#include "engine_game_object.hpp"
#include "engine_keyboardmovement.hpp"
#include "engine_model.hpp"
#include "engine_swapchain.hpp"
#include "engine_texture.hpp"
#include "engine_ui.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "imgui_impl_sdl3.h"
#include "offscreenSystem.hpp"
#include "pbrMaterials.hpp"
#include "point_light_system.hpp"
#include "shaderList.hpp"
#include "simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cassert>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace GameEngine {

EngineApp::EngineApp()
{
  std::vector<PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4}};

  globalPool = EngineDescriptorPoolGrowable::Builder(geDevice)
                   .setNumSets(MAX_DESCRIPTOR_SET)
                   .addPoolSizeRatioVector(sizes)
                   .build();

  loadGameObjects(*globalPool);
}

EngineApp::~EngineApp() {}

void EngineApp::run()
{
  // loading uboBuffer
  std::vector<std::unique_ptr<EngineBuffer>> uboBuffers(
      EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] =
        std::make_unique<EngineBuffer>(geDevice,
                                       sizeof(GlobalUbo),
                                       1,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
    uboBuffers[i]->map();
  }

  // loading in all the descriptorSetLayouts
  DescriptorSetLayouts descriptorSetLayouts{};
  populateDescriptorSetLayouts(descriptorSetLayouts);

  // skybox texture
  auto cubeMap = EngineTexture::createCubeMap(geDevice, cubeTextureFilePaths);

  // creating descriptor sets
  DescriptorSets descriptorSets{};

  descriptorSets.uboSets.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < descriptorSets.uboSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    EngineDescriptorWriter(*descriptorSetLayouts.uboSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(descriptorSets.uboSets[i]);
  }

  {
    auto imageInfo = cubeMap->getDescriptorInfo();
    EngineDescriptorWriter(*descriptorSetLayouts.cubemap, *globalPool)
        .writeImage(0, &imageInfo)
        .build(descriptorSets.cubeMap);
  }

  // passing in the rest to method to handle the iteration and population
  // of remaining descroptorsets
  populateMatTexDescriptorSets(descriptorSets, descriptorSetLayouts);

  // initializing ui object
  EngineUI ui{geRenderer};

  std::vector<VkDescriptorSetLayout> mainLayouts{
      descriptorSetLayouts.uboSetLayout->getDescriptorSetLayout(),
      descriptorSetLayouts.materialSetLayout->getDescriptorSetLayout(),
      descriptorSetLayouts.cubemap->getDescriptorSetLayout(),
      descriptorSetLayouts.textureLayout->getDescriptorSetLayout(),
  };

  SimpleRenderSystem simpleRenderSystem{geDevice,
                                        mainShaderFiles,
                                        mainLayouts,
                                        descriptorSets.uboSets,
                                        descriptorSets.materialSets,
                                        descriptorSets.cubeMap,
                                        descriptorSets.textureArray,
                                        geRenderer.getPipelineRenderingInfo()};

  // only need to pass in the uboSetLayout to this because no material or
  // texture data is needed
  std::vector<VkDescriptorSetLayout> pointLightLayouts{mainLayouts[0]};

  PointLightSystem pointLightSystem{geDevice,
                                    pointLightShaderFiles,
                                    pointLightLayouts,
                                    descriptorSets.uboSets,
                                    geRenderer.getPipelineRenderingInfo()};

  // only need ubo and cubemap textures
  std::vector<VkDescriptorSetLayout> cubeMapLayouts{mainLayouts[0],
                                                    mainLayouts[2]};
  CubeMapRenderSystem cubeMapRender{geDevice,
                                    cubeMapShaderFiles,
                                    cubeMapLayouts,
                                    descriptorSets.uboSets,
                                    descriptorSets.cubeMap,
                                    geRenderer.getPipelineRenderingInfo()};

  OffscreenSystem offScreenSystem{geDevice,
                                  offscreenShaderFiles,
                                  pointLightLayouts,
                                  descriptorSets.uboSets,
                                  offscreenRenderer.getPipelineRenderingInfo()};

  std::vector<VkDescriptorSetLayout> bboxLayout{mainLayouts[0]};
  BboxRenderer bboxRender{geDevice,
                          bboxShaderFiles,
                          bboxLayout,
                          descriptorSets.uboSets,
                          geRenderer.getPipelineRenderingInfo()};

  // FIXME:
  BVHAccel bvhaccel{geObjects, geDevice, *globalPool};

  // FIXME:

  EngineCamera camera{};
  camera.setViewDirection(glm::vec3(0.0f), glm::vec3(0.f, 0.f, 1.f));

  auto viewerObject = GameObject::createGameObject();
  viewerObject.transform.translation.z = -2.5f;
  EngineController cameraController{};

  auto currentTime = std::chrono::high_resolution_clock::now();

  bool shouldQuit = false;
  SDL_Event e;

  bool userSeeMouse = false;
  uint32_t x, y = 0;

  while (!shouldQuit) {
    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;

    // fps counter
    float fps = 1.f / frameTime;

    int lastFrame = 0;

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
          }
          else {
            userSeeMouse = false;
            SDL_SetWindowRelativeMouseMode(geWindow.getSDLWindow(), true);
          }
        }
      }

      if (e.type == SDL_EVENT_MOUSE_MOTION && !userSeeMouse) {
        cameraController.handleMouseMovements(e, frameTime, viewerObject);
      }

      if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
          hasClicked = true;
          float pixelDensity =
              SDL_GetWindowPixelDensity(geWindow.getSDLWindow());
          x = (int)pixelDensity * e.motion.x;
          y = (int)pixelDensity * e.motion.y;
        }
      }

      if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        geWindow.setWindowResizedFlag();
      }

      ImGui_ImplSDL3_ProcessEvent(&e);
    }

    updateScene();

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
      geRenderer.beginRender(commandBuffer);

      simpleRenderSystem.render(frameInfo);
      pointLightSystem.render(frameInfo);
      cubeMapRender.render(frameInfo);

      if (showbbox) {
        bboxRender.render(frameInfo);
      }

      /*
      if (hasClicked) {
        id_t objIDFromPixel = offscreenRenderer.getPixelData<uint32_t>(x, y);

        std::cout << objIDFromPixel << std::endl;

        if (objIDFromPixel > 0) {
          geObjects.at(objIDFromPixel).isSelected = true;
        }
        hasClicked = false;
      }
      */

      // ui
      if (userSeeMouse) {
        ui.newFrame();
        ui.bvhUI(showbbox);

        for (auto& kv : geObjects) {
          auto& obj = kv.second;
          if (obj.isSelected) {
            if (obj.model == nullptr) {
              glm::vec3 color{obj.color};
              float intensity = obj.pointLight->lightIntensity;
              ui.renderLightUI(color, intensity);
              obj.color = {color.r, color.g, color.b};
              obj.pointLight->lightIntensity = intensity;
            }
            else {
              TransformComponent transformUI{obj.transform};
              ui.renderModelUI(transformUI);
              obj.transform = transformUI;
            }
          }
        }
        ui.render(commandBuffer);
        ui.endFrame();
      }
      // ui end

      // drawing objects offscreen
      if (auto commandBuffer_off = offscreenRenderer.beginFrame(frameIndex)) {
        frameInfo.commandBuffer = commandBuffer_off;
        offscreenRenderer.beginRender(commandBuffer_off);
        frameInfo.commandBuffer = commandBuffer_off;
        offScreenSystem.render(frameInfo);
        offscreenRenderer.endRender(commandBuffer_off);
        offscreenRenderer.endFrame();
      }

      geRenderer.endRender(commandBuffer);
      geRenderer.endFrame();

      lastFrame = frameIndex;
    }
  }

  vkDeviceWaitIdle(geDevice.device());
}

void EngineApp::populateDescriptorSetLayouts(
    DescriptorSetLayouts& descriptorSetLayouts)
{
  // set layout for pipeline
  descriptorSetLayouts.uboSetLayout =
      EngineDescriptorSetLayout::Builder(geDevice)
          .addBinding(0,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .build();
  // cubeMapSetLayout
  descriptorSetLayouts.cubemap =
      EngineDescriptorSetLayout::Builder(geDevice)
          .addBinding(0,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  // materialSetLayout
  descriptorSetLayouts.materialSetLayout =
      EngineDescriptorSetLayout::Builder(geDevice)
          .addBinding(0,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  const VkDescriptorBindingFlags flags =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;  // VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
                                                                  // removed

  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags{};
  bindingFlags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  bindingFlags.bindingCount = 1;
  bindingFlags.pBindingFlags = &flags;

  descriptorSetLayouts.textureLayout =
      EngineDescriptorSetLayout::Builder(geDevice)
          .addBinding(0,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT,
                      EngineTexture::MAX_TEXTURES)
          .setFlags(
              VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT)
          .setpNext(&bindingFlags)
          .build();
}

void EngineApp::updateScene()
{
  for (auto& kv : geObjects) {
    auto& obj = kv.second;

    if (obj.model == nullptr) continue;

    for (auto& topNode : obj.model->topNodes) {
      topNode->Draw(obj.transform.mat4());
    }
  }
}
void EngineApp::populateMatTexDescriptorSets(
    DescriptorSets& descriptorSets,
    DescriptorSetLayouts& descriptorSetLayouts)
{
  descriptorSets.materialSets.resize(PBRMaterial::return_total_material());

  int materialCount = 0;
  std::vector<VkDescriptorImageInfo> imageInfos;

  for (auto& kv : geObjects) {
    auto& obj = kv.second;

    if (obj.model == nullptr) continue;

    uint32_t textureOffset = imageInfos.size();
    for (std::shared_ptr<EngineTexture>& texture : obj.model->images) {
      imageInfos.push_back(texture->getDescriptorInfo());
    }

    for (std::shared_ptr<PBRMaterial>& material : obj.model->materials) {
      // doing this here because we need to know the total amount of
      // textures before we can determine offset and possibly no
      // point in fluhsing data twice
      material->properties.offset = textureOffset;

      {
        auto& buffer = material->getMaterialBuffer();
        buffer =
            std::make_shared<EngineBuffer>(geDevice,
                                           sizeof(MaterialProperties),
                                           1,
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffer.get()->map();
        buffer.get()->writeToBuffer(&material->properties);
        buffer.get()->flush();
      }

      auto bufferInfo = material->getMaterialBuffer()->descriptorInfo();
      EngineDescriptorWriter(*descriptorSetLayouts.materialSetLayout,
                             *globalPool)
          .writeBuffer(0, &bufferInfo)
          .build(descriptorSets.materialSets[materialCount]);

      materialCount++;
    }
  }

  {
    uint32_t descriptorCount[1] = {static_cast<uint32_t>(imageInfos.size())};
    VkDescriptorSetVariableDescriptorCountAllocateInfo
        variableDescripAllocInfo{};
    variableDescripAllocInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableDescripAllocInfo.descriptorSetCount = 1;
    variableDescripAllocInfo.pDescriptorCounts = descriptorCount;
    EngineDescriptorWriter(*descriptorSetLayouts.textureLayout, *globalPool)
        .writeBulkImage(0, imageInfos)
        .build(descriptorSets.textureArray, &variableDescripAllocInfo);
  }
}

void EngineApp::loadGameObjects(EngineDescriptorPoolGrowable& growablePool)
{
  std::cout << "Loading Game Objects..." << std::endl;

  std::shared_ptr<EngineModel> geModel =
      EngineModel::createModelFromFile(geDevice,
                                       "./models/just_a_girl.glb",
                                       growablePool);
  auto girl = GameObject::createGameObject();
  girl.model = geModel;
  girl.transform.scale = {.01f, .01f, .01f};
  girl.transform.translation = {0.0f, -1.f, 0.0f};
  girl.transform.rotation = {glm::radians(0.f), glm::radians(180.f), 0.0f};

  geObjects.emplace(girl.getID(), std::move(girl));

  geModel = EngineModel::createModelFromFile(geDevice,
                                             "./models/shiba.glb",
                                             growablePool);
  auto shiba = GameObject::createGameObject();
  shiba.model = geModel;
  shiba.transform.translation = {-1.0f, -0.5f, -0.2f};
  shiba.transform.scale = {0.5f, 0.5, 0.5f};
  shiba.transform.rotation = {glm::radians(180.f),
                              glm::radians(-30.f),
                              glm::radians(180.f)};

  geObjects.emplace(shiba.getID(), std::move(shiba));

  geModel = EngineModel::createModelFromFile(geDevice,
                                             "./models/the_bathroom_free.glb",
                                             growablePool);
  auto background = GameObject::createGameObject();
  background.model = geModel;
  background.transform.scale = {1.5f, 1.5f, 1.5f};
  background.transform.translation = {0.0f, -1.0f, 1.0f};
  geObjects.emplace(background.getID(), std::move(background));

  /*
  geModel = EngineModel::createModelFromFile(
      geDevice,
      "./models/free_1975_porsche_911_930_turbo.glb",
      growablePool);
  auto car = GameObject::createGameObject();
  car.model = geModel;
  car.transform.translation = {-5.0, 0.f, -1.0f};
  geObjects.emplace(car.getID(), std::move(car));
  */

  std::vector<glm::vec3> lightColors = {{0.2f, 0.2f, .7f}, {0.2f, 0.2f, .7f}};

  for (int i = 0; i < lightColors.size(); i++) {
    auto pointLight = GameObject::makePointLight(10.f);
    pointLight.color = lightColors[i];

    auto rotateLight =
        glm::rotate(glm::mat4(1.0f),
                    (i * glm::two_pi<float>()) / lightColors.size(),
                    {0.f, 1.f, 0.f});

    pointLight.transform.translation =
        glm::vec3(rotateLight * glm::vec4(-1.f, 1.f, 1.f, 1.f));

    geObjects.emplace(pointLight.getID(), std::move(pointLight));
  }

  std::cout << "Game Objects Loaded!" << std::endl;
}
}  // namespace GameEngine
