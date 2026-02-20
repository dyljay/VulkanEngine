#include "engine_renderer.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include <stdexcept>

namespace GameEngine {

EngineRenderer::EngineRenderer(EngineWindow &window, EngineDevice &device)
    : geWindow{window}, geDevice{device} {
  recreateSwapChain();
  createCommandBuffers();
}

EngineRenderer::~EngineRenderer() { freeCommandBuffers(); }

void EngineRenderer::createCommandBuffers() {
  commandBuffers.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};

  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = geDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(geDevice.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void EngineRenderer::freeCommandBuffers() {
  vkFreeCommandBuffers(geDevice.device(), geDevice.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

void EngineRenderer::recreateSwapChain() {
  int width = 0, height = 0;
  // auto extent = geWindow.getExtent();
  SDL_GetWindowSize(geWindow.getSDLWindow(), &width, &height);

  while (width == 0 || height == 0) {
    SDL_GetWindowSize(geWindow.getSDLWindow(), &width, &height);
    // extent = geWindow.getExtent();
    SDL_WaitEvent(nullptr);
  }

  geWindow.setWindowDimensions();
  auto extent = geWindow.getExtent();

  vkDeviceWaitIdle(geDevice.device());

  if (geSwapChain == nullptr) {
    geSwapChain = std::make_unique<EngineSwapChain>(geDevice, extent);
  } else {
    std::shared_ptr<EngineSwapChain> oldSwapChain = std::move(geSwapChain);
    geSwapChain =
        std::make_unique<EngineSwapChain>(geDevice, extent, oldSwapChain);

    if (!oldSwapChain->compareSwapFormats(*geSwapChain.get())) {
      throw std::runtime_error(
          "swap chain image (or depth) format has changed!");
    }
  }
}

VkCommandBuffer EngineRenderer::beginFrame() {
  assert(!isFrameStarted && "Can't call beginFrame while already in progress");

  if (geWindow.wasWindowResized()) {
    geWindow.resetWindowResizedFlag();
    recreateSwapChain();
    return nullptr;
  }

  auto result = geSwapChain->acquireNextImage(&currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  isFrameStarted = true;

  auto commandBuffer = getCurrentCommandBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  return commandBuffer;
}

void EngineRenderer::endFrame() {
  assert(isFrameStarted &&
         "Can't call endFrame while frame is not in progress");

  auto commandBuffer = getCurrentCommandBuffer();
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }

  auto result =
      geSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  isFrameStarted = false;
  currentFrameIndex =
      (currentFrameIndex + 1) % EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
}
void EngineRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "Can't call beginSwapChainRenderPass if frame is not in progress");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Can't begin render pass on a command buffer from a different frame");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = geSwapChain->getRenderPass();
  renderPassInfo.framebuffer = geSwapChain->getFrameBuffer(currentImageIndex);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = geSwapChain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(geSwapChain->getSwapChainExtent().width);
  viewport.height =
      static_cast<float>(geSwapChain->getSwapChainExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0}, geSwapChain->getSwapChainExtent()};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void EngineRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "Can't call endSwapChainRenderPass if frame is not in progress");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Can't end render pass on a command buffer from a different frame");

  vkCmdEndRenderPass(commandBuffer);
}

} // namespace GameEngine
