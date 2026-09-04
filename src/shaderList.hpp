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
    .vertexShader = "shaders/id_obj.vert.spv",
    .fragShader = "shaders/id_obj.frag.spv",
};

const Shader bboxShaderFiles{
    .vertexShader = "shaders/bbox.vert.spv",
    .fragShader = "shaders/bbox.frag.spv",
};

const Shader outlineShaderFiles{.vertexShader = "shaders/outline.vert.spv",
                                .fragShader = "shaders/outline.frag.spv"};
}  // namespace GameEngine
