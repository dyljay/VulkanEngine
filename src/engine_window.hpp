#pragma once

#define GLFW_INCLUDE_VULKAN

#include <string>
#include <GLFW/glfw3.h>

namespace GameEngine {

class EngineWindow {
public:
    EngineWindow(int width, int height, std::string windowName);
    ~EngineWindow();
    
    EngineWindow(const EngineWindow &) = delete;
    EngineWindow &operator=(const EngineWindow &) = delete;
    
    bool shouldClose();
    
    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
    
    VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
    
    bool wasWindowResized() { return frameBufferResized; }
    void resetWindowResizedFlag() { frameBufferResized = false; }
    
    GLFWwindow* getGLFWWindow() const { return window; }

private:
    static void frameBufferResizedCallback(GLFWwindow *window, int width, int height);
    void initWindow();
    
    int width;
    int height;
    bool frameBufferResized = false;
    
    GLFWwindow* window;
    std::string windowName;
};
}


