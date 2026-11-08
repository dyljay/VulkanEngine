#include "engine_app.hpp"

#include <cstdlib>
#include <iostream>

int main() {
  GameEngine::EngineApp app{};

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
