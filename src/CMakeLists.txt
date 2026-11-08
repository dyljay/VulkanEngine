# Engine sources, split across subfolders of src/ (core, asset, bvh, io, systems, ui, util).
# Pulled in from the root CMakeLists.txt with add_subdirectory(src) after the
# VulkanEngine target is created.

# every .cpp under src/, re-globbed at build time so new files don't need a manual re-configure
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${ENGINE_SOURCES})

# add every folder under src/ as an include dir so flat includes like "engine_device.hpp" still resolve
file(GLOB_RECURSE ENGINE_DIRS LIST_DIRECTORIES true
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
foreach(DIR ${ENGINE_DIRS})
  if(IS_DIRECTORY ${DIR})
    target_include_directories(${PROJECT_NAME} PUBLIC ${DIR})
  endif()
endforeach()
