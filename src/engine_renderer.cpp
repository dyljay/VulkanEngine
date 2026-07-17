#include "engine_renderer.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "src/engine_device.hpp"
#include "src/engine_image.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <iostream>
#include <memory>
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
  SDL_GetWindowSize(geWindow.getSDLWindow(), &width, &height);

  while (width == 0 || height == 0) {
    SDL_GetWindowSize(geWindow.getSDLWindow(), &width, &height);
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

/// ----- Offscreen Renderer ----- ///
OffScreenRenderer::OffScreenRenderer(EngineWindow &window, EngineDevice &device)
    : geWindow{window}, geDevice{device} {
  init();
}

OffScreenRenderer::~OffScreenRenderer() {
  freeResources();
  freeCommandBuffer();
}

void OffScreenRenderer::init() {
  createOffscreenImage();
  createDepthResources();
  createRenderPass();
  createFramebuffer();
  createSyncObject();
  createCommandBuffer();
}

void OffScreenRenderer::createCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};

  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = geDevice.getCommandPool();
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(geDevice.device(), &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void OffScreenRenderer::freeResources() {
  vkDestroyFramebuffer(geDevice.device(), frameBuffer, nullptr);

  vkDestroyRenderPass(geDevice.device(), renderPass, nullptr);

  vkDestroySemaphore(geDevice.device(), imageAvailableSemaphore, nullptr);

  vkDestroyFence(geDevice.device(), imageRenderedFence, nullptr);
}

void OffScreenRenderer::freeCommandBuffer() {
  vkFreeCommandBuffers(geDevice.device(), geDevice.getCommandPool(), 1,
                       &commandBuffer);
}

void OffScreenRenderer::beginRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "Can't call beginSwapChainRenderPass if frame is not in progress");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frameBuffer;

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {offscreenImage->getWidth(),
                                      offscreenImage->getHeight()};

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
  viewport.width = static_cast<float>(imageExtent.width);
  viewport.height = static_cast<float>(imageExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0},
                   {offscreenImage->getWidth(), offscreenImage->getHeight()}};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void OffScreenRenderer::endRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

VkResult OffScreenRenderer::submitCommandBuffer() {
  if (imageRenderedFence != VK_NULL_HANDLE) {
    vkWaitForFences(geDevice.device(), 1, &imageRenderedFence, VK_TRUE,
                    UINT64_MAX);
  }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  submitInfo.signalSemaphoreCount = 0;

  vkResetFences(geDevice.device(), 1, &imageRenderedFence);
  auto result = vkQueueSubmit(geDevice.graphicsQueue(), 1, &submitInfo,
                              imageRenderedFence);
  return result;
}

VkCommandBuffer OffScreenRenderer::beginFrame() {
  assert(!isFrameStarted && "Can't call beginFrame while already in progress");

  if (geWindow.wasWindowResized()) {
    geWindow.resetWindowResizedFlag();
    init();
    return nullptr;
  }

  isFrameStarted = true;

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  return commandBuffer;
}

void OffScreenRenderer::endFrame() {
  assert(isFrameStarted &&
         "Can't call endFrame while frame is not in progress");

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }

  auto result = submitCommandBuffer();

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  isFrameStarted = false;
}

void OffScreenRenderer::createOffscreenImage() {
  imageExtent.width = geWindow.getWidth();
  imageExtent.height = geWindow.getHeight();

  ImageConfigInfo imageInfo{};
  imageInfo.extent.width = imageExtent.width;
  imageInfo.extent.height = imageExtent.height;
  imageInfo.format = VK_FORMAT_R32_UINT;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  ImageViewConfigInfo imageViewInfo{};

  offscreenImage =
      std::make_unique<EngineImage>(geDevice, imageInfo, imageViewInfo);
}

void OffScreenRenderer::createDepthResources() {
  ImageConfigInfo imageInfo{};
  imageInfo.extent.width = imageExtent.width;
  imageInfo.extent.height = imageExtent.height;
  imageInfo.format = VK_FORMAT_D32_SFLOAT;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.mipLevels = 1;
  imageInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  ImageViewConfigInfo viewInfo{};
  viewInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

  depthImage = std::make_unique<EngineImage>(geDevice, imageInfo, viewInfo);
}

void OffScreenRenderer::createFramebuffer() {
  std::array<VkImageView, 2> attachments = {offscreenImage->getImageView(),
                                            depthImage->getImageView()};

  VkFramebufferCreateInfo framebufferInfo = {};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  framebufferInfo.pAttachments = attachments.data();
  framebufferInfo.width = imageExtent.width;
  framebufferInfo.height = imageExtent.height;
  framebufferInfo.layers = 1;

  if (vkCreateFramebuffer(geDevice.device(), &framebufferInfo, nullptr,
                          &frameBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create framebuffer!");
  }
}

void OffScreenRenderer::createRenderPass() {
  VkAttachmentDescription depthAttachment{};

  // TODO: double check if this is the correct format
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = offscreenImage->getImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.srcAccessMask = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstSubpass = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (auto result = vkCreateRenderPass(geDevice.device(), &renderPassInfo,
                                       nullptr, &renderPass) != VK_SUCCESS) {
    std::cout << result << std::endl;
    throw std::runtime_error("failed to create render pass!");
  }
}

void OffScreenRenderer::createSyncObject() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(geDevice.device(), &semaphoreInfo, nullptr,
                        &imageAvailableSemaphore) != VK_SUCCESS) {
    throw std::runtime_error("failed to create off-screen semaphore");
  }

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateFence(geDevice.device(), &fenceInfo, nullptr,
                    &imageRenderedFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to create fence for off-screen renderer");
  }
}
} // namespace GameEngine
