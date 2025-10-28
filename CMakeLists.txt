cmake_minimum_required(VERSION 3.15)

# Allow older dependencies to work with newer CMake
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

# Fix GLM compatibility with newer CMake versions
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Set minimum policy version for subdirectories to avoid GLM errors
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

project(Ymirge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(YMIRGE_BUILD_TESTS "Build tests" ON)
option(YMIRGE_ENABLE_SIMD "Enable SIMD optimizations (AVX2)" ON)
option(YMIRGE_BUILD_UI "Build with raylib UI (Phase 3)" OFF)  # OFF by default - use SDL UI
option(YMIRGE_BUILD_SDL_UI "Build with SDL2 + ImGui UI" ON)

# Find dependencies
find_package(Threads REQUIRED)

# Fetch raylib if UI is enabled
if(YMIRGE_BUILD_UI)
    include(FetchContent)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )

    # Don't build raylib examples
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(raylib)

    message(STATUS "raylib UI enabled")
else()
    message(STATUS "raylib UI disabled (console-only build)")
endif()

# Fetch SDL2, ImGui, and OpenGL if SDL UI is enabled
if(YMIRGE_BUILD_SDL_UI)
    include(FetchContent)

    # SDL2
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)

    # Dear ImGui (docking branch)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)

    # Create ImGui library
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC SDL2::SDL2-static)

    # glad (OpenGL loader) - auto-download pre-generated files
    set(GLAD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include")
    set(GLAD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/src")

    # Download glad files if they don't exist
    if(NOT EXISTS "${GLAD_INCLUDE_DIR}/glad/glad.h")
        message(STATUS "Downloading pre-generated GLAD files for OpenGL 3.3 Core...")

        file(MAKE_DIRECTORY "${GLAD_INCLUDE_DIR}/glad")
        file(MAKE_DIRECTORY "${GLAD_INCLUDE_DIR}/KHR")
        file(MAKE_DIRECTORY "${GLAD_SRC_DIR}")

        # Download from a gist with pre-generated OpenGL 3.3 Core files
        # These are generated from glad.dav1d.de with: Language=C/C++, gl=3.3, Profile=Core
        set(GLAD_BASE_URL "https://raw.githubusercontent.com/Dav1dde/glad/glad2/")

        # Note: Using glad2 branch which has proper file structure
        file(DOWNLOAD
            "https://raw.githubusercontent.com/Dav1dde/glad/c/include/glad/glad.h"
            "${GLAD_INCLUDE_DIR}/glad/glad.h"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS_H
        )

        file(DOWNLOAD
            "https://raw.githubusercontent.com/Dav1dde/glad/c/include/KHR/khrplatform.h"
            "${GLAD_INCLUDE_DIR}/KHR/khrplatform.h"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS_KHR
        )

        file(DOWNLOAD
            "https://raw.githubusercontent.com/Dav1dde/glad/c/src/glad.c"
            "${GLAD_SRC_DIR}/glad.c"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS_C
        )

        list(GET DOWNLOAD_STATUS_H 0 STATUS_CODE_H)
        list(GET DOWNLOAD_STATUS_C 0 STATUS_CODE_C)

        if(NOT STATUS_CODE_H EQUAL 0 OR NOT STATUS_CODE_C EQUAL 0)
            message(WARNING "Failed to download GLAD files from GitHub. Creating minimal implementation...")
            # Will create minimal glad files below
            set(CREATE_MINIMAL_GLAD TRUE)
        else()
            message(STATUS "GLAD files downloaded successfully")
        endif()
    else()
        message(STATUS "Using existing GLAD files")
    endif()

    add_library(glad STATIC "${GLAD_SRC_DIR}/glad.c")
    target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIR})

    # Find OpenGL
    find_package(OpenGL REQUIRED)

    # GLM (OpenGL Mathematics library - header-only)
    # Set minimum policy version before fetching GLM to avoid compatibility errors
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
    )
    set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glm)

    message(STATUS "SDL2 + ImGui + OpenGL UI enabled")
else()
    message(STATUS "SDL2 UI disabled")
endif()

# GLM is now needed for core algorithms (RiverEnhancements)
# Moved outside SDL_UI check so it's always available

include(FetchContent)

# GLM (OpenGL Mathematics library - header-only)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

FetchContent_MakeAvailable(glm)
set(STB_IMAGE_WRITE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image_write.h")
if(NOT EXISTS "${STB_IMAGE_WRITE_PATH}")
    message(STATUS "Downloading stb_image_write.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"
        "${STB_IMAGE_WRITE_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(STATUS_CODE EQUAL 0)
        message(STATUS "stb_image_write.h downloaded successfully")
    else()
        message(WARNING "Failed to download stb_image_write.h - you may need to download it manually")
    endif()
else()
    message(STATUS "stb_image_write.h already exists")
endif()

# Download tinyexr.h for EXR export (Phase 1 Enhancement)
set(TINYEXR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyexr.h")
if(NOT EXISTS "${TINYEXR_PATH}")
    message(STATUS "Downloading tinyexr.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/syoyo/tinyexr/master/tinyexr.h"
        "${TINYEXR_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(STATUS_CODE EQUAL 0)
        message(STATUS "tinyexr.h downloaded successfully")
    else()
        message(WARNING "Failed to download tinyexr.h - you may need to download it manually")
    endif()
else()
    message(STATUS "tinyexr.h already exists")
endif()

# Download miniz.h for compression (required by tinyexr)
set(MINIZ_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniz.h")
if(NOT EXISTS "${MINIZ_PATH}")
    message(STATUS "Downloading miniz.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h"
        "${MINIZ_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS_MINIZ
    )
    list(GET DOWNLOAD_STATUS_MINIZ 0 STATUS_CODE_MINIZ)
    if(STATUS_CODE_MINIZ EQUAL 0)
        message(STATUS "miniz.h downloaded successfully")
    else()
        message(WARNING "Failed to download miniz.h - you may need to download it manually")
    endif()
else()
    message(STATUS "miniz.h already exists")
endif()

# Download nlohmann/json for layer serialization (Phase 2.1)
set(NLOHMANN_JSON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann/json.hpp")
if(NOT EXISTS "${NLOHMANN_JSON_PATH}")
    message(STATUS "Downloading nlohmann/json...")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp"
        "${NLOHMANN_JSON_PATH}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS_JSON
    )
    list(GET DOWNLOAD_STATUS_JSON 0 STATUS_CODE_JSON)
    if(STATUS_CODE_JSON EQUAL 0)
        message(STATUS "nlohmann/json downloaded successfully")
    else()
        message(WARNING "Failed to download nlohmann/json - you may need to download it manually")
    endif()
else()
    message(STATUS "nlohmann/json already exists")
endif()

# Source files
set(YMIRGE_CORE_SOURCES
    src/core/HeightMap.cpp
    src/core/PerlinNoise.cpp
    src/core/ThreadPool.cpp
    src/core/TerrainGenerator.cpp
    src/core/ResolutionManager.cpp
    src/core/UndoStack.cpp
    src/core/HeightMapEditCommand.cpp
)

set(YMIRGE_CORE_HEADERS
    src/core/HeightMap.h
    src/core/PerlinNoise.h
    src/core/ThreadPool.h
    src/core/TerrainGenerator.h
    src/core/TerrainParams.h
    src/core/ResolutionManager.h
    src/core/UndoCommand.h
    src/core/UndoStack.h
    src/core/HeightMapEditCommand.h
)

set(YMIRGE_ALGORITHM_SOURCES
    src/algorithms/ValleyFlattening.cpp
    src/algorithms/EdgeSmoothing.cpp
    src/algorithms/Peaks.cpp
    src/algorithms/Rivers.cpp
    src/algorithms/TerrainSoftening.cpp
    src/algorithms/ValleyConnectivity.cpp
    src/algorithms/ThermalErosion.cpp
    src/algorithms/HydraulicErosion.cpp
    src/algorithms/RiverEnhancements.cpp
)

set(YMIRGE_ALGORITHM_HEADERS
    src/algorithms/ValleyFlattening.h
    src/algorithms/EdgeSmoothing.h
    src/algorithms/Peaks.h
    src/algorithms/Rivers.h
    src/algorithms/TerrainSoftening.h
    src/algorithms/ValleyConnectivity.h
    src/algorithms/ThermalErosion.h
    src/algorithms/HydraulicErosion.h
    src/algorithms/RiverEnhancements.h
)

set(YMIRGE_EXPORT_SOURCES
    src/export/ImageExporter.cpp
    src/export/AdvancedSplatmap.cpp
)

set(YMIRGE_EXPORT_HEADERS
    src/export/ImageExporter.h
    src/export/AdvancedSplatmap.h
)

set(YMIRGE_TOOL_SOURCES
    src/tools/BrushManager.cpp
    src/tools/StampTool.cpp
    src/tools/PolygonSelection.cpp
    src/tools/PathTool.cpp
)

set(YMIRGE_TOOL_HEADERS
    src/layers/LayerCommand.cpp
    src/tools/BrushTool.h
    src/tools/RaiseBrush.h
    src/tools/LowerBrush.h
    src/tools/SmoothBrush.h
    src/tools/FlattenBrush.h
    src/tools/BrushManager.h
    src/tools/StampTool.h
    src/tools/PolygonSelection.h
    src/tools/PathTool.h
)

set(YMIRGE_LAYER_SOURCES
    src/layers/TerrainLayer.cpp
    src/layers/LayerStack.cpp
    src/layers/LayerSerializer.cpp
    src/layers/LayerThumbnail.cpp
    src/layers/LayerBase.cpp
    src/layers/LayerGroup.cpp
    src/layers/LayerCommand.cpp
)

set(YMIRGE_LAYER_HEADERS
    src/layers/TerrainLayer.h
    src/layers/LayerStack.h
    src/layers/LayerSerializer.h
    src/layers/LayerThumbnail.h
    src/layers/LayerBase.h
    src/layers/LayerGroup.h
    src/layers/LayerCommand.h
)

set(YMIRGE_UI_SOURCES
    src/rendering/TerrainRenderer.cpp
    src/ui/Slider.cpp
    src/ui/PresetManager.cpp
    src/ui/UIManager.cpp
)

set(YMIRGE_UI_HEADERS
    src/rendering/TerrainRenderer.h
    src/ui/Slider.h
    src/ui/PresetManager.h
    src/ui/UIManager.h
)

# SDL2 UI source files
set(YMIRGE_SDL_UI_SOURCES
    src/rendering/TerrainRendererGL.cpp
    src/rendering/Camera3D.cpp
    src/rendering/Shader.cpp
    src/ui/PresetManager.cpp
    src/ui/UIManagerImGui.cpp
    src/gpu/GPUCompute.cpp
    src/gpu/ComputeShader.cpp
    src/gpu/GPUBuffer.cpp
    src/gpu/GPUTest.cpp
    src/gpu/PerlinNoiseGPU.cpp
    src/gpu/GaussianBlurGPU.cpp
)

set(YMIRGE_SDL_UI_HEADERS
    src/rendering/TerrainRendererGL.h
    src/rendering/Camera3D.h
    src/rendering/Shader.h
    src/ui/PresetManager.h
    src/ui/UIManagerImGui.h
    src/gpu/GPUCompute.h
    src/gpu/ComputeShader.h
    src/gpu/GPUBuffer.h
    src/gpu/GPUTest.h
    src/gpu/PerlinNoiseGPU.h
    src/gpu/GaussianBlurGPU.h
)

# SDL2 + ImGui UI Application
if(YMIRGE_BUILD_SDL_UI)
    add_executable(ymirge
        src/main_ui_sdl.cpp
        ${YMIRGE_CORE_SOURCES}
        ${YMIRGE_CORE_HEADERS}
        ${YMIRGE_ALGORITHM_SOURCES}
        ${YMIRGE_ALGORITHM_HEADERS}
        ${YMIRGE_EXPORT_SOURCES}
        ${YMIRGE_EXPORT_HEADERS}
        ${YMIRGE_TOOL_SOURCES}
        ${YMIRGE_TOOL_HEADERS}
        ${YMIRGE_LAYER_SOURCES}
        ${YMIRGE_LAYER_HEADERS}
        ${YMIRGE_SDL_UI_SOURCES}
        ${YMIRGE_SDL_UI_HEADERS}
    )

    target_include_directories(ymirge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}/src/export
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tools
        ${CMAKE_CURRENT_SOURCE_DIR}/src/layers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    )

    target_link_libraries(ymirge PRIVATE
        Threads::Threads
        SDL2::SDL2-static
        SDL2::SDL2main
        imgui
        glad
        glm::glm
        OpenGL::GL
    )

    target_compile_definitions(ymirge PRIVATE YMIRGE_SDL_UI_ENABLED)

    set_target_properties(ymirge PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Copy shaders
    add_custom_command(TARGET ymirge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/shaders
            $<TARGET_FILE_DIR:ymirge>/shaders
        COMMENT "Copying shaders"
    )

    # Copy GPU shaders
    add_custom_command(TARGET ymirge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/shaders
            $<TARGET_FILE_DIR:ymirge>/gpu_shaders
        COMMENT "Copying GPU shaders"
    )

    # Compiler flags
    if(MSVC)
        target_compile_options(ymirge PRIVATE /W4 /O2 /permissive-)
        if(YMIRGE_ENABLE_SIMD)
            target_compile_options(ymirge PRIVATE /arch:AVX2)
        endif()
    else()
        target_compile_options(ymirge PRIVATE -Wall -Wextra -Wpedantic -O3)
        if(YMIRGE_ENABLE_SIMD)
            target_compile_options(ymirge PRIVATE -mavx2 -mfma)
        endif()
    endif()
endif()

# Copy assets
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Configuration
message(STATUS "Ymirge Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${YMIRGE_BUILD_TESTS}")
message(STATUS "  SIMD Support: ${YMIRGE_ENABLE_SIMD}")
message(STATUS "  SDL2 UI: ${YMIRGE_BUILD_SDL_UI}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
