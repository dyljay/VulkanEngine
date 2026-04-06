#include "pbrMaterials.hpp"

namespace GameEngine {

int PBRMaterial::total_material_count = 0;

void PBRMaterial::increment_total_material(int count) {
  total_material_count += count;
}

}; // namespace GameEngine
