cmake_minimum_required(VERSION 3.20.0)
set(NAME VulkanEngine)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${NAME} VERSION 0.0.1)

# Find Vulkan package

# Link against Vulkan
#find_package(glfw3 REQUIRED)

#find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

set(VULKAN_INCLUDE_DIR "/Users/dylanjayasundera/VulkanSDK/1.4.328.1/macOS/include/")
set(VULKAN_LIB_DIR "/Users/dylanjayasundera/VulkanSDK/1.4.328.1/macOS/lib")

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory("${PROJECT_SOURCE_DIR}/lib/sdl/vendored/SDL" EXCLUDE_FROM_ALL)
add_subdirectory("${PROJECT_SOURCE_DIR}/lib/fastgltf/")

# For macOS, you might need to add MoltenVK-specific configuration
#if(APPLE)
#  # Add MoltenVK framework path
#  set(MOLTENVK_PATH "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
#  target_link_libraries(your_app PUBLIC "-framework Metal" "-framework
#    MetalKit" "-framework Cocoa")

# If using MoltenVK as a framework
# target_link_libraries(your_app PUBLIC "${MOLTENVK_PATH}")
#endif()

#message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
#if (NOT Vulkan_FOUND)
#  message(FATAL_ERROR "Could not find Vulkan Library!")
#else()
#  message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
#endif()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

set(CMAKE_BUILD_TYPE Debug)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
"${CMAKE_SOURCE_DIR}/build")

message(STATUS "CREATING BUILD")
target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR})


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

