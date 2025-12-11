#pragma once

#include "engine_window.hpp"
#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "engine_descriptor.hpp"

// std
#include <memory>
#include <vector>

namespace GameEngine {
class EngineApp {
public:
    void run();
    void updateGameObjects(float deltaTime);
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;
    
    EngineApp();
    ~EngineApp();
    
    EngineApp(const EngineApp&) = delete;
    EngineApp &operator=(const EngineApp&) = delete;
    
private:
    void loadGameObjects();
    
    EngineWindow geWindow{WIDTH, HEIGHT, "Skumpwit"};
    EngineDevice geDevice{geWindow};
    EngineRenderer geRenderer{geWindow, geDevice};
    
    std::unique_ptr<EngineDescriptorPool> globalPool{};
    GameObject::Map geObjects;
};
}

