#pragma once

#include "system.hpp"

namespace GameEngine {

const Shader mainShaderFiles{
    .vertexShader = "shaders/vert.spv",
    .fragShader = "shaders/frag.spv",
};

const Shader pointLightShaderFiles{
    .vertexShader = "shaders/point_light.vert.spv",
    .fragShader = "shaders/point_light.frag.spv",
};

const Shader cubeMapShaderFiles{
    .vertexShader = "shaders/cubemap_vert.vert.spv",
    .fragShader = "shaders/cubemap_frag.frag.spv",
};

const Shader offscreenShaderFiles{
    .vertexShader = "id_obj.vert.spv",
    .fragShader = "id_obj.frag.spv",
};

} // namespace GameEngine
