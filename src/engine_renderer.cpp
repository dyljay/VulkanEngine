#include "engine_renderer.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "engine_buffer.hpp"
#include "engine_swapchain.hpp"
#include "sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"
#include "src/engine_device.hpp"
#include "src/engine_image.hpp"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

EngineRenderer::EngineRenderer(EngineWindow& window, EngineDevice& device)
    : geWindow{window},
      geDevice{device}
{
  recreateSwapChain();
  createCommandBuffers();
  createPipelineCreateInfo();
}

EngineRenderer::~EngineRenderer() { freeCommandBuffers(); }

void EngineRenderer::createCommandBuffers()
{
  commandBuffers.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};

  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = geDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(geDevice.device(),
                               &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void EngineRenderer::createPipelineCreateInfo()
{
  pipelineCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineCreate.pNext = VK_NULL_HANDLE;

  colorAttachmentFormats = {geSwapChain->getSwapChainImageFormat()};

  pipelineCreate.colorAttachmentCount = colorAttachmentFormats.size();
  pipelineCreate.pColorAttachmentFormats = colorAttachmentFormats.data();
  pipelineCreate.depthAttachmentFormat = geSwapChain->getDepthImageFormat();

  if (EngineImage::HasStencilComponent(geSwapChain->getDepthImageFormat())) {
    pipelineCreate.stencilAttachmentFormat = geSwapChain->getDepthImageFormat();
  }
}

void EngineRenderer::freeCommandBuffers()
{
  vkFreeCommandBuffers(geDevice.device(),
                       geDevice.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

void EngineRenderer::recreateSwapChain()
{
  int width = 0, height = 0;
  SDL_GetWindowSizeInPixels(geWindow.getSDLWindow(), &width, &height);

  while (width == 0 || height == 0) {
    SDL_GetWindowSizeInPixels(geWindow.getSDLWindow(), &width, &height);
    SDL_WaitEvent(nullptr);
  }

  geWindow.setWindowDimensions();
  auto extent = geWindow.getExtent();

  vkDeviceWaitIdle(geDevice.device());

  if (geSwapChain == nullptr) {
    geSwapChain = std::make_unique<EngineSwapChain>(geDevice, extent);
  }
  else {
    std::shared_ptr<EngineSwapChain> oldSwapChain = std::move(geSwapChain);
    geSwapChain =
        std::make_unique<EngineSwapChain>(geDevice, extent, oldSwapChain);

    if (!oldSwapChain->compareSwapFormats(*geSwapChain.get())) {
      throw std::runtime_error(
          "swap chain image (or depth) format has "
          "changed!");
    }
  }
}

VkCommandBuffer EngineRenderer::beginFrame()
{
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

void EngineRenderer::endFrame()
{
  assert(isFrameStarted &&
         "Can't call endFrame while frame is not in "
         "progress");

  auto commandBuffer = getCurrentCommandBuffer();

  geSwapChain->transitionPresentImageLayout(commandBuffer, currentImageIndex);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }

  auto result =
      geSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  isFrameStarted = false;
  currentFrameIndex =
      (currentFrameIndex + 1) % EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void EngineRenderer::beginRender(VkCommandBuffer commandBuffer)
{
  assert(isFrameStarted &&
         "Can't call beginSwapChainRenderPass if frame "
         "is not in "
         "progress");

  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Can't begin render pass on a command buffer "
         "from a "
         "different "
         "frame");

  geSwapChain->transitionImageLayouts(commandBuffer, currentImageIndex);

  VkRenderingAttachmentInfo colorAttachmentInfos = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = geSwapChain->getResourceView(currentImageIndex),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
      .resolveImageView = geSwapChain->getImageView(currentImageIndex),
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {0.0, 0.0, 0.0},
  };

  VkRenderingAttachmentInfo depthAttachmentInfos = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = geSwapChain->getDepthView(currentImageIndex),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {1.0, 0.0},
  };

  VkRenderingInfo renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea.extent = geSwapChain->getSwapChainExtent();
  renderInfo.renderArea.offset = {0, 0};
  renderInfo.layerCount = 1;
  renderInfo.viewMask = 0;
  std::vector<VkRenderingAttachmentInfo> colorAttachments = {
      colorAttachmentInfos};

  renderInfo.colorAttachmentCount = colorAttachments.size();
  renderInfo.pColorAttachments = colorAttachments.data();
  renderInfo.pDepthAttachment = &depthAttachmentInfos;

  vkCmdBeginRendering(commandBuffer, &renderInfo);

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

void EngineRenderer::endRender(VkCommandBuffer commandBuffer)
{
  assert(isFrameStarted &&
         "Can't call endSwapChainRenderPass if frame is "
         "not in "
         "progress");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Can't end render pass on a command buffer from a "
         "different frame");

  vkCmdEndRendering(commandBuffer);
}

/// ----- Offscreen Renderer ----- ///
OffScreenRenderer::OffScreenRenderer(EngineWindow& window, EngineDevice& device)
    : geWindow{window},
      geDevice{device}
{
  init();
}

OffScreenRenderer::~OffScreenRenderer()
{
  freeResources();
  freeCommandBuffer();
}

void OffScreenRenderer::init()
{
  createOffscreenImage();
  createDepthResources();
  createSyncObject();
  createCommandBuffer();
  createPipelineRenderingCreateInfo();
}

void OffScreenRenderer::createCommandBuffer()
{
  commandBuffers.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};

  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = geDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(geDevice.device(),
                               &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffer");
  }
}

void OffScreenRenderer::freeResources()
{
  for (int i = 0; i < imageRenderedFences.size(); i++) {
    vkDestroyFence(geDevice.device(), imageRenderedFences[i], nullptr);
  }
}

void OffScreenRenderer::freeCommandBuffer()
{
  vkFreeCommandBuffers(geDevice.device(),
                       geDevice.getCommandPool(),
                       commandBuffers.size(),
                       commandBuffers.data());
}

void OffScreenRenderer::beginRender(VkCommandBuffer commandBuffer)
{
  EngineImage::ImageMemoryBarrier(
      commandBuffer,
      offscreenImages[currentIndex]->getImage(),
      offscreenImages[currentIndex]->getImageFormat(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  EngineImage::ImageMemoryBarrier(
      commandBuffer,
      depthImages[currentIndex]->getImage(),
      depthImages[currentIndex]->getImageFormat(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkRenderingAttachmentInfo colorAttachmentInfos = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = offscreenImages[currentIndex]->getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {0.0, 0.0, 0.0},
  };

  VkRenderingAttachmentInfo depthAttachmentInfos = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depthImages[currentIndex]->getImageView(),
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {1.0, 0.0},
  };

  VkRenderingInfo renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea.offset = {0, 0};
  renderInfo.renderArea.extent = {offscreenImages[currentIndex]->getWidth(),
                                  offscreenImages[currentIndex]->getHeight()};
  renderInfo.layerCount = 1;
  renderInfo.viewMask = 0;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = &colorAttachmentInfos;
  renderInfo.pDepthAttachment = &depthAttachmentInfos;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
  clearValues[1].depthStencil = {1.0f, 0};

  vkCmdBeginRendering(commandBuffer, &renderInfo);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(imageExtent.width);
  viewport.height = static_cast<float>(imageExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0},
                   {offscreenImages[currentIndex]->getWidth(),
                    offscreenImages[currentIndex]->getHeight()}};

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void OffScreenRenderer::endRender(VkCommandBuffer commandBuffer)
{
  vkCmdEndRendering(commandBuffer);
}

VkResult OffScreenRenderer::submitCommandBuffer()
{
  /*
  if (imageRenderedFence != VK_NULL_HANDLE) {
    vkWaitForFences(geDevice.device(),
                    1,
                    &imageRenderedFence,
                    VK_TRUE,
                    UINT64_MAX);
  }
  */

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentIndex];

  /*
  vkResetFences(geDevice.device(), 1, &imageRenderedFence);
  */
  auto result =
      vkQueueSubmit(geDevice.graphicsQueue(), 1, &submitInfo, nullptr);
  return result;
}

VkCommandBuffer OffScreenRenderer::beginFrame(int imageIndex)
{
  if (geWindow.wasWindowResized()) {
    geWindow.resetWindowResizedFlag();
    init();
    return nullptr;
  }
  isFrameStarted = true;

  currentIndex = imageIndex;

  VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer");
  }

  return commandBuffer;
}

void OffScreenRenderer::endFrame()
{
  assert(isFrameStarted &&
         "Can't call endFrame while frame is not in "
         "progress");

  EngineImage::ImageMemoryBarrier(
      commandBuffers[currentIndex],
      offscreenImages[currentIndex]->getImage(),
      offscreenImages[currentIndex]->getImageFormat(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  if (vkEndCommandBuffer(commandBuffers[currentIndex]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer");
  }

  auto result = submitCommandBuffer();

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    init();
  }
  else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  isFrameStarted = false;
}

void OffScreenRenderer::createOffscreenImage()
{
  offscreenImages.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < offscreenImages.size(); i++) {
    int w, h;
    SDL_GetWindowSizeInPixels(geWindow.getSDLWindow(), &w, &h);
    imageExtent.width = w;
    imageExtent.height = h;

    ImageConfigInfo imageInfo{};
    imageInfo.extent.width = imageExtent.width;
    imageInfo.extent.height = imageExtent.height;
    imageInfo.format = VK_FORMAT_R32_UINT;
    imageInfo.usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageViewConfigInfo imageViewInfo{};

    offscreenImages[i] =
        std::make_unique<EngineImage>(geDevice, imageInfo, imageViewInfo);
  }
}

void OffScreenRenderer::createPipelineRenderingCreateInfo()
{
  pipelineCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineCreate.pNext = VK_NULL_HANDLE;

  attachmentFormat = offscreenImages[0]->getImageFormat();

  pipelineCreate.colorAttachmentCount = 1;
  pipelineCreate.pColorAttachmentFormats = &attachmentFormat;
  pipelineCreate.depthAttachmentFormat = depthImages[0]->getImageFormat();
}

void OffScreenRenderer::createDepthResources()
{
  depthImages.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (int i = 0; i < depthImages.size(); i++) {
    ImageConfigInfo imageInfo{};
    imageInfo.extent.width = imageExtent.width;
    imageInfo.extent.height = imageExtent.height;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.mipLevels = 1;
    imageInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ImageViewConfigInfo viewInfo{};
    viewInfo.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    depthImages[i] =
        std::make_unique<EngineImage>(geDevice, imageInfo, viewInfo);
  }
}

uint32_t OffScreenRenderer::readPixelValue(uint32_t x, uint32_t y)
{
  vkWaitForFences(geDevice.device(),
                  1,
                  &imageRenderedFences[currentIndex],
                  VK_TRUE,
                  UINT64_MAX);

  uint32_t size = imageExtent.width * imageExtent.height * sizeof(uint32_t);

  EngineBuffer buffer =
      EngineBuffer{geDevice,
                   size,
                   1,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                   1,
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT};

  VkCommandBuffer commandBuffer = geDevice.beginSingleTimeCommands();
  EngineImage::ImageMemoryBarrier(commandBuffer,
                                  offscreenImages[currentIndex]->getImage(),
                                  attachmentFormat,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  geDevice.endSingleTimeCommands(commandBuffer);

  geDevice.copyImageToBuffer(offscreenImages[currentIndex]->getImage(),
                             buffer.getBuffer(),
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             imageExtent.width,
                             imageExtent.height,
                             1);

  commandBuffer = geDevice.beginSingleTimeCommands();
  EngineImage::ImageMemoryBarrier(commandBuffer,
                                  offscreenImages[currentIndex]->getImage(),
                                  attachmentFormat,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  geDevice.endSingleTimeCommands(commandBuffer);

  buffer.map();
  void* mapped = buffer.getMappedMemory();
  uint32_t* idMemory = static_cast<uint32_t*>(mapped);

  uint32_t pixelID = idMemory[y * imageExtent.width + x];

  return pixelID;
}

void OffScreenRenderer::createSyncObject()
{
  imageRenderedFences.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < imageRenderedFences.size(); i++) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(geDevice.device(),
                      &fenceInfo,
                      nullptr,
                      &imageRenderedFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error(
          "failed to create fence for off-screen "
          "renderer");
    }
  }
}
}  // namespace GameEngine
