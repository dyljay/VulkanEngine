include(envUnix.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.20.0)
set(NAME VulkanEngine)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${NAME} VERSION 0.20.0)

if (DEFINED VULKAN_SDK_PATH)
  set(VULKAN_INCLUDE_DIR "${VULKAN_SDK_PATH}/include/")
  set(VULKAN_LIB_DIR "${VULKAN_SDK_PATH}/lib")
  set(Vulkan_FOUND "True")
endif()

find_package(Vulkan REQUIRED)

# This assumes the SDL source is available in vendored/SDL
add_subdirectory("${PROJECT_SOURCE_DIR}/lib/sdl/vendored/SDL" EXCLUDE_FROM_ALL)
add_subdirectory("${PROJECT_SOURCE_DIR}/lib/fastgltf/")

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp
                          ${PROJECT_SOURCE_DIR}/src/render_sysetms/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

set(CMAKE_BUILD_TYPE Debug)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
"${CMAKE_SOURCE_DIR}/build")

message(STATUS "CREATING BUILD")
target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/lib
  ${PROJECT_SOURCE_DIR}/src/systems
)

target_include_directories(VulkanEngine PRIVATE ${VULKAN_INCLUDE_DIR})
target_link_directories(VulkanEngine PRIVATE ${VULKAN_LIB_DIR})

# ImGUI
add_library(imgui
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_vulkan.cpp
    lib/imgui/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
    lib/imgui
    lib/imgui/backends
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan imgui SDL3::SDL3 fastgltf)


############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
find_program(GLSL_VALIDATOR glslangValidator HINTS
  ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
  /usr/bin
  /usr/local/bin
  ${VULKAN_SDK_PATH}/Bin
  ${VULKAN_SDK_PATH}/Bin32
  $ENV{VULKAN_SDK}/Bin/
  $ENV{VULKAN_SDK}/Bin32/
)

# get all .vert, .frag and .comp files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/shaders/*.frag"
  "${PROJECT_SOURCE_DIR}/shaders/*.vert"
  "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
  if(FILE_NAME STREQUAL "single_radixsort.comp")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.1 ${GLSL} -o ${SPIRV}
      DEPENDS ${GLSL})
  else()
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
      DEPENDS ${GLSL})
  endif()
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)
