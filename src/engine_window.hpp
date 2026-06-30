#pragma once

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include <vulkan/vulkan.h>

#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace GameEngine {

class EngineWindow {
public:
  EngineWindow(int width, int height, std::string windowName);
  ~EngineWindow();

  EngineWindow(const EngineWindow &) = delete;
  EngineWindow &operator=(const EngineWindow &) = delete;

  bool shouldClose(SDL_Event &e);

  void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }

  bool wasWindowResized() { return frameBufferResized; }
  void setWindowResizedFlag() { frameBufferResized = true; }
  void resetWindowResizedFlag() { frameBufferResized = false; }

  void setWindowDimensions();

  SDL_Window *getSDLWindow() const { return window; }

  static constexpr SDL_WindowFlags windowFlags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_HIGH_PIXEL_DENSITY);

private:
  void initWindow();

  int width;
  int height;
  bool frameBufferResized = false;

  SDL_Window *window;
  std::string windowName;
  bool isInitialized = false;
};
} // namespace GameEngine
