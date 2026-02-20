#include "engine_window.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "vulkan/vulkan_core.h"

namespace GameEngine {

EngineWindow::EngineWindow(int width, int height, std::string windowName)
    : width{width}, height{height}, windowName{windowName} {
  initWindow();
}

EngineWindow::~EngineWindow() {
  if (isInitialized) {
    SDL_DestroyWindow(window);
  }
}

bool EngineWindow::shouldClose(SDL_Event &e) {
  if (e.type == SDL_EVENT_QUIT) {
    return true;
  }
  return false;
}

void EngineWindow::initWindow() {
  SDL_Init(SDL_INIT_VIDEO);

  // TODO: should error check here
  window = SDL_CreateWindow(windowName.c_str(), width, height, windowFlags);

  isInitialized = true;
  SDL_SetWindowRelativeMouseMode(window, true);
}

void EngineWindow::createWindowSurface(VkInstance instance,
                                       VkSurfaceKHR *surface) {
  if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, surface)) {
    throw std::runtime_error("failed to create surface!");
  }
}

void EngineWindow::setWindowDimensions() {
  int w, h;

  SDL_GetWindowSize(window, &w, &h);

  width = w;
  height = h;
}
/*
void EngineWindow::frameBufferResizedCallback(GLFWwindow *window, int width,
                                              int height) {
  auto geWindow =
      reinterpret_cast<EngineWindow *>(glfwGetWindowUserPointer(window));
  geWindow->frameBufferResized = true;
  geWindow->width = width;
  geWindow->height = height;
}
*/
} // namespace GameEngine
