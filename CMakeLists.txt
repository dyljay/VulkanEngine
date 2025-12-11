cmake_minimum_required(VERSION 3.20.0)
set(NAME VulkanEngine)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${NAME} VERSION 0.0.1)

# Find Vulkan package

# Link against Vulkan
find_package(glfw3 REQUIRED)

find_package(Vulkan REQUIRED)

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

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
"${CMAKE_SOURCE_DIR}/build")

message(STATUS "CREATING BUILD")
target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)



