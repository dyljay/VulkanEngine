#include "engine_ui.hpp"

#include "engine_game_object.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "src/engine_descriptor.hpp"
#include "src/engine_device.hpp"
#include "src/engine_renderer.hpp"
#include "src/engine_swapchain.hpp"
#include "src/engine_window.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

EngineUI::EngineUI(EngineRenderer& geRenderer)
    : geRenderer{geRenderer}
{
  initImGUIPool();
  initUI();
}

EngineUI::~EngineUI()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void EngineUI::initImGUIPool()
{
  // unfortunately have to do this as per IMGUI documentation
  std::vector<VkDescriptorPoolSize> pool_sizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  imguiPool =
      EngineDescriptorPool::Builder(geRenderer.getDevice())
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
          .setMaxSets(1000)
          .addPoolSize(pool_sizes)
          .build();
}

void EngineUI::initUI()
{
  ImGui::CreateContext();

  ImGui_ImplSDL3_InitForVulkan(geRenderer.getWindow().getSDLWindow());

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = geRenderer.getDevice().instance();
  initInfo.PhysicalDevice = geRenderer.getDevice().getPhysicalDevice();
  initInfo.Device = geRenderer.getDevice().device();
  initInfo.Queue = geRenderer.getDevice().graphicsQueue();
  initInfo.DescriptorPool = imguiPool->getVkPool();
  initInfo.MinImageCount = EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
  initInfo.PipelineInfoMain.Subpass = 0;
  initInfo.PipelineInfoMain.MSAASamples =
      geRenderer.getDevice().getSampleCount();
  initInfo.CheckVkResultFn = check_vk_result;

  // dynamic rendering info
  initInfo.UseDynamicRendering = true;
  ImGui_ImplVulkan_PipelineInfo pipelinInfo{};
  pipelinInfo.PipelineRenderingCreateInfo =
      geRenderer.getPipelineRenderingInfo();
  initInfo.PipelineInfoMain = pipelinInfo;

  ImGui_ImplVulkan_Init(&initInfo);
}

void EngineUI::newFrame()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void EngineUI::render(VkCommandBuffer commandBuffer)
{
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

// don't forget to call ImGUI::Begin() and ImGui::End() here. shouldn't
// call in function in case you need to render multiple
void EngineUI::renderLightUI(glm::vec3& color, float& intensity)
{
  if (ImGui::Begin("Light Color")) {
    ImGui::Text("RGB");
    ImGui::SliderFloat("Red", &color.r, 0.0f, 1.0f);
    ImGui::SliderFloat("Green", &color.g, 0.0f, 1.0f);
    ImGui::SliderFloat("Blue", &color.b, 0.0f, 1.0f);
    ImGui::Text("Intensity");
    ImGui::SliderFloat("Intensity", &intensity, 0.f, 10.f);
    ImGui::End();
  }
}

void EngineUI::renderModelUI(TransformComponent& transform)
{
  if (ImGui::Begin("Model Transform")) {
    ImGui::DragFloat3("Location", &transform.translation.x, 0.01f);
    ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.01f);
    ImGui::DragFloat3("Scale", &transform.scale.x, 0.001f, 0.0f, 10.0f);
    ImGui::End();
  }
}

void EngineUI::endFrame() { ImGui::EndFrame(); }
}  // namespace GameEngine
