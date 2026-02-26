#include "engine_keyboardmovement.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include "glm/fwd.hpp"
#include "src/engine_game_object.hpp"

namespace GameEngine {

void EngineController::moveInXYZPlane(float dt, GameObject &gameObject) {
  const bool *keyState = SDL_GetKeyboardState(nullptr);

  float yaw = gameObject.transform.rotation.y;
  const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
  const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
  const glm::vec3 upDir{0.f, 1.f, 0.f};

  glm::vec3 moveDir{0.f};
  if (keyState[SDL_SCANCODE_W])
    moveDir += forwardDir;
  if (keyState[SDL_SCANCODE_S])
    moveDir -= forwardDir;
  if (keyState[SDL_SCANCODE_SPACE])
    moveDir += upDir;
  if (keyState[SDL_SCANCODE_LSHIFT])
    moveDir -= upDir;
  if (keyState[SDL_SCANCODE_D])
    moveDir += rightDir;
  if (keyState[SDL_SCANCODE_A])
    moveDir -= rightDir;

  if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
    gameObject.transform.translation +=
        moveSpeed * dt * glm::normalize(moveDir);
  }
}

void EngineController::handleMouseMovements(SDL_Event &e, float dt,
                                            GameObject &gameObject) {
  // No normalization, so no need to check for zero
  gameObject.transform.rotation.y += e.motion.xrel * sensitivity;
  gameObject.transform.rotation.x += (e.motion.yrel * sensitivity);

  gameObject.transform.rotation.x =
      glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
  gameObject.transform.rotation.y =
      glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
}
} // namespace GameEngine
