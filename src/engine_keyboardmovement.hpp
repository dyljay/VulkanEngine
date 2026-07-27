#pragma once

#include <memory>
#include <vector>

#include "SDL3/SDL_events.h"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"

namespace GameEngine {

class EngineController {
   public:
    // scan codes used to index into keyboard state array to see if
    // the key is pressed
    struct KeyStates {
        int moveLeft = SDLK_A;
        int moveRight = SDLK_D;
        int moveForward = SDLK_W;
        int moveBackward = SDLK_S;
        int moveUp = SDLK_E;
        int moveDown = SDLK_Q;
    };

    void moveInXYZPlane(float dt, GameObject& gameObject);

    void handleMouseMovements(SDL_Event& e,
                              float dt,
                              GameObject& gameObject);

    KeyStates keys{};
    float moveSpeed{1.5f};
    float lookSpeed{1.5f};
    float sensitivity{1.f / 200.f};

   private:
    VkResult render();
    std::vector<VkImage> image;
};

}  // namespace GameEngine
