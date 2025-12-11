#include "engine_window.hpp"

namespace GameEngine {

EngineWindow::EngineWindow(int width, int height, std::string windowName) : width{width}, height{height}, windowName{windowName} {
    initWindow();
}

EngineWindow::~EngineWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool EngineWindow::shouldClose() {
    return glfwWindowShouldClose(window);
}

void EngineWindow::initWindow() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizedCallback);
}

void EngineWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create surface!");
    }
}

void EngineWindow::frameBufferResizedCallback(GLFWwindow *window, int width, int height) {
    auto geWindow = reinterpret_cast<EngineWindow *>(glfwGetWindowUserPointer(window));
    geWindow->frameBufferResized = true;
    geWindow->width = width;
    geWindow->height = height;
}

}


