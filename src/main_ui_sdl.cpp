#ifdef YMIRGE_SDL_UI_ENABLED

#include <SDL2/SDL.h>

// Undefine Windows min/max macros that conflict with std::min/max
// Must be done right after SDL.h (which includes Windows.h on Windows)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "TerrainRendererGL.h"
#include "UIManagerImGui.h"
#include "ResolutionManager.h"
#include "UndoStack.h"
#include "HeightMapEditCommand.h"
#include "BrushManager.h"
#include "StampTool.h"
#include "LayerStack.h"
#include "LayerCommand.h"
#include "LayerSerializer.h"
#include "ImageExporter.h"
#include "ThreadPool.h"
#include "GPUCompute.h"
#include "GPUTest.h"
#include "PerlinNoiseGPU.h"
#include "GaussianBlurGPU.h"
#include "PerlinNoise.h"

#include <memory>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <thread>

// stb_image functions (defined in StampTool.cpp)
extern "C" {
    unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels);
    unsigned short* stbi_load_16(const char* filename, int* x, int* y, int* channels_in_file, int desired_channels);
    void stbi_image_free(void* retval_from_stbi_load);
}

// GPU test and benchmark functions
void runGPUTests() {
    if (!GPUCompute::isAvailable()) {
        std::cout << "GPU tests skipped (compute not available)" << std::endl;
        return;
    }

    std::cout << "\n=== GPU Tests ===" << std::endl;

    // Test 1: Simple add value test
    {
        std::cout << "\n[Test 1] Add Value Test" << std::endl;
        HeightMap testMap(256, 256);
        for (int i = 0; i < testMap.getWidth() * testMap.getHeight(); i++) {
            testMap.getData()[i] = 0.5f;
        }

        if (GPUTest::testAddValue(testMap, 0.25f)) {
            float avg = 0.0f;
            for (int i = 0; i < testMap.getWidth() * testMap.getHeight(); i++) {
                avg += testMap.getData()[i];
            }
            avg /= (testMap.getWidth() * testMap.getHeight());
            std::cout << "  Average height after +0.25: " << avg << " (expected: 0.75)" << std::endl;

            if (std::abs(avg - 0.75f) < 0.01f) {
                std::cout << "  ✓ Test PASSED" << std::endl;
            } else {
                std::cout << "  ✗ Test FAILED" << std::endl;
            }
        }
    }

    // Test 2: Perlin Noise comparison
    {
        std::cout << "\n[Test 2] Perlin Noise Correctness" << std::endl;
        const int size = 256;
        HeightMap cpuMap(size, size);
        HeightMap gpuMap(size, size);

        PerlinNoise cpuNoise(12345);
        PerlinNoiseGPU gpuNoise;

        // Generate CPU version
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float nx = float(x) / size * 2.0f;
                float ny = float(y) / size * 2.0f;
                float value = cpuNoise.octaveNoise(nx, ny, 4, 0.5f, 2.0f);
                cpuMap.at(x, y) = (value + 1.0f) * 0.5f;
            }
        }

        // Generate GPU version
        gpuNoise.generate(gpuMap, 2.0f, 4, 0.5f, 2.0f, 12345);

        // Compare results
        float maxDiff = 0.0f;
        float avgDiff = 0.0f;
        for (int i = 0; i < size * size; i++) {
            float diff = std::abs(cpuMap.getData()[i] - gpuMap.getData()[i]);
            maxDiff = (std::max)(maxDiff, diff);
            avgDiff += diff;
        }
        avgDiff /= (size * size);

        std::cout << "  Average difference: " << avgDiff << std::endl;
        std::cout << "  Max difference: " << maxDiff << std::endl;

        if (avgDiff < 0.01f && maxDiff < 0.1f) {
            std::cout << "  ✓ Test PASSED (GPU matches CPU within tolerance)" << std::endl;
        } else {
            std::cout << "  ⚠ Results differ (may be due to precision/implementation)" << std::endl;
        }
    }
}

void runGPUBenchmarks() {
    if (!GPUCompute::isAvailable()) {
        std::cout << "GPU benchmarks skipped (compute not available)" << std::endl;
        return;
    }

    std::cout << "\n=== GPU Benchmarks ===" << std::endl;

    const uint32_t seed = 12345;
    const float scale = 2.0f;
    const int octaves = 4;
    const float persistence = 0.5f;
    const float lacunarity = 2.0f;

    PerlinNoise cpuNoise(seed);
    PerlinNoiseGPU gpuNoise;

    auto testResolution = [&](int size) {
        std::cout << "\n[" << size << "x" << size << "]" << std::endl;

        HeightMap cpuMap(size, size);
        HeightMap gpuMap(size, size);

        // CPU benchmark
        auto cpuStart = std::chrono::high_resolution_clock::now();
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float nx = float(x) / size * scale;
                float ny = float(y) / size * scale;
                float value = cpuNoise.octaveNoise(nx, ny, octaves, persistence, lacunarity);
                cpuMap.at(x, y) = (value + 1.0f) * 0.5f;
            }
        }
        auto cpuEnd = std::chrono::high_resolution_clock::now();
        auto cpuTime = std::chrono::duration_cast<std::chrono::milliseconds>(cpuEnd - cpuStart).count();

        // GPU benchmark
        auto gpuStart = std::chrono::high_resolution_clock::now();
        gpuNoise.generate(gpuMap, scale, octaves, persistence, lacunarity, seed);
        auto gpuEnd = std::chrono::high_resolution_clock::now();
        auto gpuTime = std::chrono::duration_cast<std::chrono::milliseconds>(gpuEnd - gpuStart).count();

        float speedup = float(cpuTime) / (std::max)(static_cast<long long>(gpuTime), 1LL);

        std::cout << "  CPU: " << cpuTime << "ms" << std::endl;
        std::cout << "  GPU: " << gpuTime << "ms" << std::endl;
        std::cout << "  Speedup: " << std::fixed << std::setprecision(1) << speedup << "x" << std::endl;
    };

    testResolution(512);
    testResolution(1024);
    testResolution(2048);

    std::cout << "\nTarget speedup: 20-30x (may vary based on hardware)" << std::endl;

    std::cout << "\n=== Gaussian Blur Benchmarks ===" << std::endl;

    GaussianBlurGPU gpuBlur;

    auto testBlurResolution = [&](int size) {
        std::cout << "\n[" << size << "x" << size << "]" << std::endl;

        const int radius = 8;
        const float sigma = 3.0f;

        HeightMap cpuMap(size, size);
        HeightMap gpuMap(size, size);

        // Initialize with noise
        PerlinNoise noise(12345);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float nx = float(x) / size * 2.0f;
                float ny = float(y) / size * 2.0f;
                float value = (noise.octaveNoise(nx, ny, 4, 0.5f, 2.0f) + 1.0f) * 0.5f;
                cpuMap.at(x, y) = value;
                gpuMap.at(x, y) = value;
            }
        }

        // CPU benchmark (simple box blur as reference)
        auto cpuStart = std::chrono::high_resolution_clock::now();
        for (int round = 0; round < 3; round++) {
            HeightMap temp = cpuMap;
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    float sum = 0.0f;
                    int count = 0;
                    for (int dy = -radius; dy <= radius; dy++) {
                        for (int dx = -radius; dx <= radius; dx++) {
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                                sum += temp.at(nx, ny);
                                count++;
                            }
                        }
                    }
                    cpuMap.at(x, y) = sum / count;
                }
            }
        }
        auto cpuEnd = std::chrono::high_resolution_clock::now();
        auto cpuTime = std::chrono::duration_cast<std::chrono::milliseconds>(cpuEnd - cpuStart).count();

        // GPU benchmark
        auto gpuStart = std::chrono::high_resolution_clock::now();
        for (int round = 0; round < 3; round++) {
            gpuBlur.blur(gpuMap, radius, sigma);
        }
        auto gpuEnd = std::chrono::high_resolution_clock::now();
        auto gpuTime = std::chrono::duration_cast<std::chrono::milliseconds>(gpuEnd - gpuStart).count();

        float speedup = float(cpuTime) / (std::max)(static_cast<long long>(gpuTime), 1LL);

        std::cout << "  CPU (box blur x3): " << cpuTime << "ms" << std::endl;
        std::cout << "  GPU (Gaussian x3): " << gpuTime << "ms" << std::endl;
        std::cout << "  Speedup: " << std::fixed << std::setprecision(1) << speedup << "x" << std::endl;
    };

    testBlurResolution(512);
    testBlurResolution(1024);
    testBlurResolution(2048);

    std::cout << "\nTarget speedup: 15-20x (may vary based on hardware)" << std::endl;
}

// Windows file dialog
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

/**
 * Modern dark theme for ImGui
 * Clean, professional look with accent colors
 */
void setupImGuiTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Base colors - dark with blue accent
    const ImVec4 bg_dark      = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    const ImVec4 bg_mid       = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    const ImVec4 bg_light     = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    const ImVec4 accent       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    const ImVec4 accent_hover = ImVec4(0.35f, 0.65f, 1.00f, 1.00f);
    const ImVec4 accent_dark  = ImVec4(0.18f, 0.44f, 0.78f, 1.00f);
    const ImVec4 text         = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
    const ImVec4 text_dim     = ImVec4(0.55f, 0.58f, 0.62f, 1.00f);
    const ImVec4 border       = ImVec4(0.25f, 0.26f, 0.28f, 1.00f);

    // Window
    colors[ImGuiCol_WindowBg]             = bg_mid;
    colors[ImGuiCol_ChildBg]              = bg_dark;
    colors[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);

    // Borders
    colors[ImGuiCol_Border]               = border;
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame (inputs, sliders)
    colors[ImGuiCol_FrameBg]              = bg_dark;
    colors[ImGuiCol_FrameBgHovered]       = bg_light;
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);

    // Title bar
    colors[ImGuiCol_TitleBg]              = bg_dark;
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]     = bg_dark;

    // Menu bar
    colors[ImGuiCol_MenuBarBg]            = bg_dark;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]          = bg_dark;
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

    // Checkmark, slider grab
    colors[ImGuiCol_CheckMark]            = accent;
    colors[ImGuiCol_SliderGrab]           = accent_dark;
    colors[ImGuiCol_SliderGrabActive]     = accent;

    // Buttons
    colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = accent_dark;

    // Headers (collapsing headers, tree nodes)
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = accent_dark;

    // Separator
    colors[ImGuiCol_Separator]            = border;
    colors[ImGuiCol_SeparatorHovered]     = accent;
    colors[ImGuiCol_SeparatorActive]      = accent;

    // Resize grip
    colors[ImGuiCol_ResizeGrip]           = ImVec4(0.26f, 0.26f, 0.30f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]    = accent;
    colors[ImGuiCol_ResizeGripActive]     = accent_hover;

    // Tabs
    colors[ImGuiCol_Tab]                  = bg_dark;
    colors[ImGuiCol_TabHovered]           = accent;
    colors[ImGuiCol_TabActive]            = accent_dark;
    colors[ImGuiCol_TabUnfocused]         = bg_dark;
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);

    // Docking (only available in imgui docking branch)
#ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview]       = ImVec4(accent.x, accent.y, accent.z, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]       = bg_dark;
#endif

    // Text
    colors[ImGuiCol_Text]                 = text;
    colors[ImGuiCol_TextDisabled]         = text_dim;

    // Plot
    colors[ImGuiCol_PlotLines]            = accent;
    colors[ImGuiCol_PlotLinesHovered]     = accent_hover;
    colors[ImGuiCol_PlotHistogram]        = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accent_hover;

    // Table
    colors[ImGuiCol_TableHeaderBg]        = bg_dark;
    colors[ImGuiCol_TableBorderStrong]    = border;
    colors[ImGuiCol_TableBorderLight]     = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);

    // Selection
    colors[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.35f);

    // Drag/drop
    colors[ImGuiCol_DragDropTarget]       = accent;

    // Nav
    colors[ImGuiCol_NavHighlight]         = accent;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    // Modal dim
    colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);

    // Style tweaks for modern look
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;

    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 12.0f;

    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);

    // Anti-aliasing
    style.AntiAliasedLines  = true;
    style.AntiAliasedFill   = true;
}

/**
 * Main SDL2 + ImGui Application
 *
 * Real-time terrain generation with interactive controls using ImGui.
 * OpenGL-based 3D rendering with modern shader pipeline.
 */
class YmirgeSDLApp {
public:
    YmirgeSDLApp()
        : window_(nullptr)
        , glContext_(nullptr)
        , screenWidth_(1600)
        , screenHeight_(900)
        , running_(true)
        , lastUpdateWasGenerating_(false)
        , compositeHeightMap_(512, 512) {

        // Initialize SDL2
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            throw std::runtime_error("Failed to initialize SDL2");
        }

        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Create window
        window_ = SDL_CreateWindow(
            "Ymirge",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screenWidth_,
            screenHeight_,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
        );

        if (!window_) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            throw std::runtime_error("Failed to create window");
        }

        // Set window icon
        {
            int iconWidth, iconHeight, iconChannels;
            unsigned char* iconData = stbi_load("assets/logo.png", &iconWidth, &iconHeight, &iconChannels, 4);
            if (iconData) {
                SDL_Surface* iconSurface = SDL_CreateRGBSurfaceFrom(
                    iconData,
                    iconWidth,
                    iconHeight,
                    32,
                    iconWidth * 4,
                    0x000000FF,
                    0x0000FF00,
                    0x00FF0000,
                    0xFF000000
                );
                if (iconSurface) {
                    SDL_SetWindowIcon(window_, iconSurface);
                    SDL_FreeSurface(iconSurface);
                }
                stbi_image_free(iconData);
            } else {
                std::cerr << "Warning: Could not load window icon from assets/logo.png" << std::endl;
            }
        }

        // Create OpenGL context
        glContext_ = SDL_GL_CreateContext(window_);
        if (!glContext_) {
            std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window_);
            SDL_Quit();
            throw std::runtime_error("Failed to create OpenGL context");
        }

        SDL_GL_MakeCurrent(window_, glContext_);
        SDL_GL_SetSwapInterval(1);  // Enable vsync

        // Initialize glad
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cerr << "Failed to initialize glad" << std::endl;
            SDL_GL_DeleteContext(glContext_);
            SDL_DestroyWindow(window_);
            SDL_Quit();
            throw std::runtime_error("Failed to initialize glad");
        }

        std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

        // Initialize GPU Compute
        GPUCompute::initialize();

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

        // HiDPI scaling - try multiple methods to detect scale
        float dpiScale = 1.0f;

        // Method 1: Check drawable vs window size ratio
        int windowW, windowH, drawableW, drawableH;
        SDL_GetWindowSize(window_, &windowW, &windowH);
        SDL_GL_GetDrawableSize(window_, &drawableW, &drawableH);
        if (windowW > 0 && drawableW > windowW) {
            dpiScale = static_cast<float>(drawableW) / static_cast<float>(windowW);
        }

        // Method 2: Check display DPI (96 is standard, 192 is 2x, etc.)
        if (dpiScale <= 1.0f) {
            float ddpi, hdpi, vdpi;
            int displayIndex = SDL_GetWindowDisplayIndex(window_);
            if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0 && ddpi > 0) {
                dpiScale = ddpi / 96.0f;
            }
        }

        // Method 3: Check GDK_SCALE environment variable (common on Linux)
        if (dpiScale <= 1.0f) {
            const char* gdkScale = std::getenv("GDK_SCALE");
            if (gdkScale) {
                dpiScale = static_cast<float>(std::atof(gdkScale));
            }
        }

        // Clamp to reasonable range and apply reduction (DPI detection often overshoots)
        dpiScale *= 0.7f;
        if (dpiScale < 1.0f) dpiScale = 1.0f;
        if (dpiScale > 4.0f) dpiScale = 4.0f;

        std::cout << "DPI Scale: " << dpiScale << std::endl;

        // Scale font for HiDPI
        float fontSize = 15.0f * dpiScale;
        io.Fonts->AddFontDefault();
        ImFontConfig fontConfig;
        fontConfig.SizePixels = fontSize;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        io.Fonts->AddFontDefault(&fontConfig);
        io.FontDefault = io.Fonts->Fonts.back();

        // Scale style for HiDPI
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);

        // Apply custom modern dark theme
        setupImGuiTheme();

        ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
        ImGui_ImplOpenGL3_Init("#version 430");

        // Create UI manager
        uiManager_ = std::make_unique<UIManagerImGui>();
        uiManager_->setDpiScale(dpiScale);

        // Create thread pool
        threadPool_ = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());

        // Create resolution manager with thread pool
        resolutionManager_ = std::make_unique<ResolutionManager>(threadPool_.get());
        resolutionManager_->setTargetResolution(Resolution::STANDARD);

        // Create undo stack (50 commands max, 100MB memory limit)
        undoStack_ = std::make_unique<UndoStack>(50, 100);
        layerUndoStack_ = std::make_unique<LayerUndoStack>(50);

        // Create brush manager
        brushManager_ = std::make_unique<BrushManager>(undoStack_.get());

        // Create stamp tool
        stampTool_ = std::make_unique<StampTool>();
        // Load the first procedural stamp (mountain)
        const auto& stamps = uiManager_->getStampLibrary().getStamps();
        if (!stamps.empty()) {
            stampTool_->loadStamp(stamps[0].filepath);
        }

        // Create layer stack (starts with 512x512, will resize when terrain generated)
        layerStack_ = std::make_unique<LayerStack>(512, 512);

        // Give UI manager access to layer stack
        uiManager_->setLayerStack(layerStack_.get());
        uiManager_->setLayerUndoStack(layerUndoStack_.get());

        // Create renderer for preview (will resize as needed)
        renderer_ = std::make_unique<TerrainRendererGL>(512, 512);

        // Generate initial terrain (async)
        resolutionManager_->generateAt(Resolution::STANDARD, uiManager_->getParams());

        // Mark that we're generating so update() will update renderer when complete
        lastUpdateWasGenerating_ = true;

        std::cout << "YmirgeSDLApp initialized successfully" << std::endl;
    }

    ~YmirgeSDLApp() {
        // Cleanup GPU Compute
        GPUCompute::shutdown();

        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // Cleanup SDL
        SDL_GL_DeleteContext(glContext_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    void run() {
        while (running_) {
            processEvents();
            update();
            render();
        }
    }

private:
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running_ = false;
            }

            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window_)) {
                running_ = false;
            }

            // Handle keyboard shortcuts
            if (event.type == SDL_KEYDOWN) {
                SDL_Keymod mod = SDL_GetModState();

                // Tool switching
                if (event.key.keysym.sym == SDLK_v) {
                    uiManager_->setActiveTool(BrushType::VIEW);
                }
                if (event.key.keysym.sym == SDLK_r) {
                    uiManager_->setActiveTool(BrushType::RAISE);
                }
                if (event.key.keysym.sym == SDLK_l) {
                    uiManager_->setActiveTool(BrushType::LOWER);
                }
                if (event.key.keysym.sym == SDLK_s && !(mod & KMOD_CTRL)) {
                    uiManager_->setActiveTool(BrushType::SMOOTH);
                }
                if (event.key.keysym.sym == SDLK_f) {
                    uiManager_->setActiveTool(BrushType::FLATTEN);
                }
                if (event.key.keysym.sym == SDLK_t) {
                    uiManager_->setActiveTool(BrushType::STAMP);
                }

                // Camera reset
                if (event.key.keysym.sym == SDLK_c) {
                    renderer_->resetCamera();
                }

                // Generate
                if (event.key.keysym.sym == SDLK_g) {
                    resolutionManager_->generateAt(Resolution::HIGH, uiManager_->getParams());
                }

                // Undo/Redo
                if (mod & KMOD_CTRL) {
                    if (event.key.keysym.sym == SDLK_z) {
                        if (undoStack_->undo()) {
                            const HeightMap& heightMap = resolutionManager_->getHeightMap();
                            bool monochrome = uiManager_->isMonochromeMode();
                            renderer_->updateTexture(heightMap, monochrome);
                        }
                    }
                    if (event.key.keysym.sym == SDLK_y) {
                        if (undoStack_->redo()) {
                            const HeightMap& heightMap = resolutionManager_->getHeightMap();
                            bool monochrome = uiManager_->isMonochromeMode();
                            renderer_->updateTexture(heightMap, monochrome);
                        }
                    }
                }
            }
        }
    }

    void update() {
        // Update UI
        uiManager_->update();

        // Update resolution manager (handles auto-upgrade)
        resolutionManager_->update();

        // Check if target resolution changed in UI
        if (uiManager_->hasResolutionChanged()) {
            resolutionManager_->setTargetResolution(uiManager_->getTargetResolution());
            uiManager_->clearResolutionChanged();
        }

        // Handle generate button
        if (uiManager_->isGenerateRequested()) {
            Resolution targetRes = uiManager_->getTargetResolution();
            resolutionManager_->generateAt(targetRes, uiManager_->getParams());
            uiManager_->clearGenerateRequested();
        }

        // Handle import/export requests
        if (uiManager_->isHeightmapImportRequested()) {
            importHeightmap();
            uiManager_->clearHeightmapImportRequested();
        }
        if (uiManager_->isHeightmapExportRequested()) {
            exportHeightmap();
            uiManager_->clearExportRequests();
        }
        if (uiManager_->isSplatmapExportRequested()) {
            exportSplatmap();
            uiManager_->clearExportRequests();
        }
        if (uiManager_->isLoadProjectRequested()) {
            loadProject();
            uiManager_->clearLoadProjectRequested();
        }
        if (uiManager_->isSaveProjectRequested()) {
            saveProject();
            uiManager_->clearSaveProjectRequested();
        }

        // Handle menu requests
        if (uiManager_->isUndoRequested()) {
            if (undoStack_->undo()) {
                const HeightMap& heightMap = resolutionManager_->getHeightMap();
                bool monochrome = uiManager_->isMonochromeMode();
                renderer_->updateTexture(heightMap, monochrome);
            }
        }
        if (uiManager_->isRedoRequested()) {
            if (undoStack_->redo()) {
                const HeightMap& heightMap = resolutionManager_->getHeightMap();
                bool monochrome = uiManager_->isMonochromeMode();
                renderer_->updateTexture(heightMap, monochrome);
            }
        }
        if (uiManager_->isClearHistoryRequested()) {
            undoStack_->clear();
        }
        if (uiManager_->isResetCameraRequested()) {
            renderer_->resetCamera();
        }
        if (uiManager_->isExitRequested()) {
            running_ = false;
        }
        uiManager_->clearMenuRequests();

        // Handle tool input (if not in View mode)
        ImGuiIO& io = ImGui::GetIO();
        BrushType activeTool = uiManager_->getActiveTool();

        if (activeTool != BrushType::VIEW && activeTool != BrushType::STAMP && !io.WantCaptureMouse) {
            // Handle brush tools (continuous strokes)
            // Update brush parameters if changed
            if (uiManager_->hasBrushChanged()) {
                brushManager_->setActiveBrush(activeTool);
                brushManager_->setBrushSize(uiManager_->getBrushSize());
                brushManager_->setBrushStrength(uiManager_->getBrushStrength());
                uiManager_->clearBrushChanged();
            }

            // Handle brush input with raycasting
            int mouseX, mouseY;
            Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            bool leftButton = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

            // Get active layer for editing
            TerrainLayer* activeLayer = layerStack_->getActiveTerrainLayer();
            if (!activeLayer || activeLayer->isLocked()) {
                // Can't edit locked layer or null layer
                if (leftButton && brushManager_->isStrokeActive()) {
                    brushManager_->endStroke();  // End stroke if layer locked
                }
            } else {
                // Layer is valid and unlocked - proceed with brush input

                // Check edit mode - are we editing heightmap or mask?
                EditMode editMode = uiManager_->getEditMode();
                HeightMap* targetMap = nullptr;

                if (editMode == EditMode::MASK) {
                    // Editing mask
                    size_t maskLayerIndex = uiManager_->getMaskEditLayerIndex();
                    TerrainLayer* maskLayer = layerStack_->getLayerAsTerrainLayer(maskLayerIndex);

                    if (maskLayer && maskLayer->hasMask()) {
                        targetMap = &maskLayer->getMask();
                    } else {
                        // No mask to edit, switch back to layer mode
                        uiManager_->setEditMode(EditMode::LAYER);
                        targetMap = &activeLayer->getHeightMap();
                    }
                } else {
                    // Editing heightmap (default)
                    targetMap = &activeLayer->getHeightMap();
                }

                // Proceed if we have a valid target
                if (targetMap) {
                    HeightMap& heightMap = *targetMap;
                ImVec4 viewportRect = uiManager_->getLastViewportRect();

                int heightMapX, heightMapY;
                bool cursorOnTerrain = renderer_->screenToHeightMapCoords(
                    heightMapX, heightMapY,
                    mouseX, mouseY,
                    (int)viewportRect.x, (int)viewportRect.y,
                    (int)viewportRect.z, (int)viewportRect.w,
                    compositeHeightMap_  // Raycast against composite for accurate cursor
                );

                if (cursorOnTerrain) {
                    // Begin stroke on mouse down
                    if (leftButton && !brushManager_->isStrokeActive()) {
                        brushManager_->beginStroke(heightMap, heightMapX, heightMapY);
                    }

                    // Apply brush during stroke
                    if (leftButton && brushManager_->isStrokeActive()) {
                        if (brushManager_->applyStroke(heightMap, heightMapX, heightMapY, io.DeltaTime)) {
                            // Recomposite layers
                            layerStack_->composite(compositeHeightMap_);

                            // Update renderer immediately for visual feedback
                            bool monochrome = uiManager_->isMonochromeMode();
                            renderer_->updateTexture(compositeHeightMap_, monochrome);
                        }
                    }

                    // End stroke on mouse up
                    if (!leftButton && brushManager_->isStrokeActive()) {
                        brushManager_->endStroke();
                    }
                }
                }  // if (targetMap)
            }
        } else if (activeTool == BrushType::STAMP && !io.WantCaptureMouse) {
            // Handle stamp tool (single click placement)
            // Load new stamp if selection changed
            if (uiManager_->hasStampChanged()) {
                const auto& stamps = uiManager_->getStampLibrary().getStamps();
                int stampIndex = uiManager_->getSelectedStampIndex();
                if (stampIndex >= 0 && stampIndex < (int)stamps.size()) {
                    stampTool_->loadStamp(stamps[stampIndex].filepath);
                    std::cout << "Loaded stamp: " << stamps[stampIndex].name << std::endl;
                }
                uiManager_->clearStampChanged();
            }

            // Update stamp parameters if changed
            if (uiManager_->hasBrushChanged()) {
                stampTool_->setBlendMode(uiManager_->getStampBlendMode());
                uiManager_->clearBrushChanged();
            }

            // Detect mouse click (down then up)
            static bool wasPressed = false;
            int mouseX, mouseY;
            Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            bool leftButton = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

            if (leftButton) {
                wasPressed = true;
            } else if (wasPressed) {
                // Click released - place stamp
                wasPressed = false;

                // Get active layer for editing
                TerrainLayer* activeLayer = layerStack_->getActiveTerrainLayer();
                if (!activeLayer || activeLayer->isLocked()) {
                    // Can't place stamp on locked layer
                    std::cout << "Cannot place stamp: layer is locked or null" << std::endl;
                } else {
                    HeightMap& heightMap = activeLayer->getHeightMap();
                    ImVec4 viewportRect = uiManager_->getLastViewportRect();

                    int heightMapX, heightMapY;
                    // Raycast against composite for accurate cursor positioning
                    bool cursorOnTerrain = renderer_->screenToHeightMapCoords(
                        heightMapX, heightMapY,
                        mouseX, mouseY,
                        (int)viewportRect.x, (int)viewportRect.y,
                        (int)viewportRect.z, (int)viewportRect.w,
                        compositeHeightMap_
                    );

                    if (cursorOnTerrain && stampTool_->isLoaded()) {
                        // Create undo command and capture affected region
                        int stampW = stampTool_->getStampWidth();
                        int stampH = stampTool_->getStampHeight();
                        float scale = uiManager_->getStampScale();
                        int radius = static_cast<int>((std::max(stampW, stampH) / 2.0f) * scale);

                        auto command = std::make_unique<HeightMapEditCommand>(&heightMap, "Stamp");
                        // Use square capture for stamps (not circular)
                        command->captureRegion(heightMapX, heightMapY, radius, true);

                        // Apply stamp
                        stampTool_->applyStamp(
                            heightMap,
                            heightMapX, heightMapY,
                            uiManager_->getStampScale(),
                            uiManager_->getStampRotation(),
                            uiManager_->getStampOpacity(),
                            uiManager_->getStampHeight()
                        );

                        // Finalize undo command
                        command->finalizeRegion();
                        undoStack_->push(std::move(command));

                        // Recomposite layers
                        layerStack_->composite(compositeHeightMap_);

                        // Update renderer with composite result
                        bool monochrome = uiManager_->isMonochromeMode();
                        renderer_->updateTexture(compositeHeightMap_, monochrome);

                        std::cout << "Stamp placed at (" << heightMapX << ", " << heightMapY << ")" << std::endl;
                    }
                }
            }
        }

        // Update camera - allow scrollwheel zoom in all modes
        int mouseX, mouseY;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

        // Get viewport bounds
        ImVec4 viewportRect = uiManager_->getLastViewportRect();
        bool mouseOverViewport = (mouseX >= viewportRect.x && mouseX < viewportRect.x + viewportRect.z &&
                                   mouseY >= viewportRect.y && mouseY < viewportRect.y + viewportRect.w);

        // Allow scrollwheel when over viewport (ignore WantCaptureMouse for scroll)
        float scrollDelta = mouseOverViewport ? io.MouseWheel : 0.0f;

        if (activeTool == BrushType::VIEW) {
            // View mode: left-drag rotates, right-drag pans, scroll zooms
            bool leftButton = !io.WantCaptureMouse && (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            bool rightButton = !io.WantCaptureMouse && (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
            renderer_->updateCamera(mouseX, mouseY, leftButton, rightButton, scrollDelta);
        } else {
            // Other modes: scroll still zooms, but no drag camera movement
            renderer_->updateCamera(mouseX, mouseY, false, false, scrollDelta);
        }

        // Update renderer if generation just completed
        if (!resolutionManager_->isGenerating() && lastUpdateWasGenerating_) {
            // Generation just completed
            const HeightMap& generatedMap = resolutionManager_->getHeightMap();

            // Check if layer stack needs resizing
            bool resizedStack = false;
            if (layerStack_->getWidth() != generatedMap.getWidth() ||
                layerStack_->getHeight() != generatedMap.getHeight()) {
                // Recreate layer stack with new dimensions
                layerStack_ = std::make_unique<LayerStack>(generatedMap.getWidth(), generatedMap.getHeight());
                uiManager_->setLayerStack(layerStack_.get());
        uiManager_->setLayerUndoStack(layerUndoStack_.get());
                compositeHeightMap_ = HeightMap(generatedMap.getWidth(), generatedMap.getHeight());
                resizedStack = true;
            }

            // Put generated terrain on active layer (or base layer if we just resized)
            TerrainLayer* targetLayer;
            if (resizedStack) {
                // After resize, always use base layer (only layer that exists)
                targetLayer = layerStack_->getLayerAsTerrainLayer(0);
                std::cout << "Generated terrain on base layer (stack resized)" << std::endl;
            } else {
                // Use currently active layer
                targetLayer = layerStack_->getActiveTerrainLayer();
                if (targetLayer) {
                    std::cout << "Generated terrain on layer: " << targetLayer->getName() << std::endl;
                }
            }

            if (targetLayer) {
                targetLayer->getHeightMap() = generatedMap;
            }

            // Composite all layers
            layerStack_->composite(compositeHeightMap_);

            // Update renderer with composite result
            bool monochrome = uiManager_->isMonochromeMode();
            renderer_->updateTexture(compositeHeightMap_, monochrome);
            renderer_->setSeaLevel(uiManager_->getParams().seaLevel);

            std::cout << "Updated renderer with "
                      << ResolutionManager::getResolutionName(resolutionManager_->getCurrentResolution())
                      << std::endl;
        }

        lastUpdateWasGenerating_ = resolutionManager_->isGenerating();

        // Handle layer compositing requests from UI
        if (uiManager_->isCompositeRequested()) {
            layerStack_->composite(compositeHeightMap_);

            // Update renderer with composite result
            bool monochrome = uiManager_->isMonochromeMode();
            renderer_->updateTexture(compositeHeightMap_, monochrome);

            uiManager_->clearCompositeRequested();
        }
    }

    void render() {
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render UI and get viewport rect
        ImVec4 viewportRect = uiManager_->render();

        // Check if parameters changed (must be after render to catch slider changes)
        if (uiManager_->hasParamsChanged()) {
            TerrainParams params = uiManager_->getParams();

            // Only generate in real-time if enabled
            if (uiManager_->isRealTimePreviewEnabled()) {
                // Generate at STANDARD (512x512) for meaningful real-time preview
                // Takes ~150-300ms which provides good feedback while dragging sliders
                resolutionManager_->generateAt(Resolution::STANDARD, params);
            }

            // Update sea level immediately (always, even if not generating)
            renderer_->setSeaLevel(params.seaLevel);

            uiManager_->clearParamsChanged();
        }

        // Render ImGui
        ImGui::Render();

        // Clear screen with dark background for UI areas
        glViewport(0, 0, screenWidth_, screenHeight_);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Clear 3D viewport area with sky blue background
        glViewport((int)viewportRect.x, (int)viewportRect.y, (int)viewportRect.z, (int)viewportRect.w);
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render 3D terrain in viewport
        renderer_->render(
            (int)viewportRect.x,
            (int)viewportRect.y,
            (int)viewportRect.z,
            (int)viewportRect.w
        );

        // Render brush cursor (if in brush mode and cursor is on terrain)
        BrushType activeTool = uiManager_->getActiveTool();
        if (activeTool != BrushType::VIEW) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            const HeightMap& heightMap = resolutionManager_->getHeightMap();
            int heightMapX, heightMapY;
            bool cursorOnTerrain = renderer_->screenToHeightMapCoords(
                heightMapX, heightMapY,
                mouseX, mouseY,
                (int)viewportRect.x, (int)viewportRect.y,
                (int)viewportRect.z, (int)viewportRect.w,
                heightMap
            );

            if (cursorOnTerrain) {
                renderer_->renderBrushCursor(
                    heightMapX, heightMapY,
                    uiManager_->getBrushSize(),
                    heightMap,
                    (int)viewportRect.z,
                    (int)viewportRect.w
                );
            }
        }

        // Render ImGui draw data
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        SDL_GL_SwapWindow(window_);
    }

    void importHeightmap() {
        // Open file dialog to select heightmap PNG
        std::string filename;

#ifdef _WIN32
        // Windows native file dialog
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "PNG Images\0*.png\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            filename = ofn.lpstrFile;
        } else {
            // User cancelled
            return;
        }
#else
        // On non-Windows, use hardcoded filename for now
        filename = "heightmap_import.png";
#endif

        std::cout << "Attempting to import heightmap from: " << filename << std::endl;

        // Load PNG using stb_image (8-bit or 16-bit)
        int width, height, channels;

        // Try 16-bit first
        unsigned short* data16 = stbi_load_16(filename.c_str(), &width, &height, &channels, 1);

        if (data16) {
            // Create new heightmap
            HeightMap newMap(width, height);

            // Convert 16-bit to float [0, 1]
            for (int i = 0; i < width * height; i++) {
                newMap.getData()[i] = static_cast<float>(data16[i]) / 65535.0f;
            }

            stbi_image_free(data16);

            // Replace current heightmap
            resolutionManager_->setHeightMap(newMap);

            // Update renderer
            bool monochrome = uiManager_->isMonochromeMode();
            renderer_->updateTexture(newMap, monochrome);

            // Clear undo history
            undoStack_->clear();

            std::cout << "Heightmap imported successfully: " << width << "x" << height << " (16-bit)" << std::endl;
            uiManager_->showExportSuccess("Heightmap imported successfully!");
            return;
        }

        // Try 8-bit
        unsigned char* data8 = stbi_load(filename.c_str(), &width, &height, &channels, 1);

        if (data8) {
            // Create new heightmap
            HeightMap newMap(width, height);

            // Convert 8-bit to float [0, 1]
            for (int i = 0; i < width * height; i++) {
                newMap.getData()[i] = static_cast<float>(data8[i]) / 255.0f;
            }

            stbi_image_free(data8);

            // Replace current heightmap
            resolutionManager_->setHeightMap(newMap);

            // Update renderer
            bool monochrome = uiManager_->isMonochromeMode();
            renderer_->updateTexture(newMap, monochrome);

            // Clear undo history
            undoStack_->clear();

            std::cout << "Heightmap imported successfully: " << width << "x" << height << " (8-bit)" << std::endl;
            uiManager_->showExportSuccess("Heightmap imported successfully!");
            return;
        }

        // Failed to load
        std::cerr << "Failed to import heightmap from: " << filename << std::endl;
        uiManager_->showExportError("Failed to import heightmap! Place heightmap_import.png in the executable directory.");
    }

    void exportHeightmap() {
        // Generate filename with timestamp
        std::time_t now = std::time(nullptr);
        std::tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        std::ostringstream oss;
        oss << "ymirge_heightmap_"
            << std::put_time(&localTime, "%Y%m%d_%H%M%S");

        // Get export format from UI
        ExportFormat format = uiManager_->getExportFormat();

        // Add appropriate extension
        switch (format) {
            case ExportFormat::PNG16:
                oss << ".png";
                break;
            case ExportFormat::RAW16:
                oss << ".raw";
                break;
            case ExportFormat::EXR:
                oss << ".exr";
                break;
            case ExportFormat::OBJ:
                oss << ".obj";
                break;
        }

        std::string filename = oss.str();

        // Export heightmap from resolution manager
        const HeightMap& heightMap = resolutionManager_->getHeightMap();
        bool success = false;

        switch (format) {
            case ExportFormat::PNG16:
                success = ImageExporter::exportHeightmap(heightMap, filename);
                break;
            case ExportFormat::RAW16:
                success = ImageExporter::exportHeightmapRAW16(heightMap, filename);
                break;
            case ExportFormat::EXR:
                // EXR export not supported in SDL2 build
                std::cerr << "EXR export is not available in the SDL2 build" << std::endl;
                success = false;
                break;
            case ExportFormat::OBJ:
                success = ImageExporter::exportMeshOBJ(heightMap, filename);
                break;
        }

        if (success) {
            std::cout << "Heightmap exported to: " << filename << std::endl;
            uiManager_->showExportSuccess("Heightmap exported successfully!");
        } else {
            std::cerr << "Heightmap export failed!" << std::endl;
            uiManager_->showExportError("Heightmap export failed!");
        }
    }

    void exportSplatmap() {
        // Generate filename with timestamp
        std::time_t now = std::time(nullptr);
        std::tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        std::ostringstream oss;
        oss << "ymirge_splatmap_"
            << std::put_time(&localTime, "%Y%m%d_%H%M%S")
            << ".png";

        std::string filename = oss.str();

        // Export splatmap from resolution manager
        const HeightMap& heightMap = resolutionManager_->getHeightMap();
        bool success = ImageExporter::exportSplatmap(heightMap, filename);

        if (success) {
            std::cout << "Splatmap exported to: " << filename << std::endl;
            uiManager_->showExportSuccess("Splatmap exported successfully!");
        } else {
            std::cerr << "Splatmap export failed!" << std::endl;
            uiManager_->showExportError("Splatmap export failed!");
        }
    }

    void loadProject() {
        // Open file dialog
        std::string filename;

#ifdef _WIN32
        // Windows native file dialog
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Ymirge Projects *.ymlayers All Files *.* ";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            filename = ofn.lpstrFile;
        } else {
            // User cancelled
            return;
        }
#else
        // On non-Windows, use hardcoded filename for now
        filename = "project.ymlayers";
#endif

        std::cout << "Attempting to load project from: " << filename << std::endl;

        // Load project
        if (LayerSerializer::load(*layerStack_, filename)) {
            // Recomposite layers
            layerStack_->composite(compositeHeightMap_);

            // Update renderer
            bool monochrome = uiManager_->isMonochromeMode();
            renderer_->updateTexture(compositeHeightMap_, monochrome);

            // Clear undo history
            layerUndoStack_->clear();
            undoStack_->clear();

            std::cout << "Project loaded successfully!" << std::endl;
            uiManager_->showExportSuccess("Project loaded successfully!");
        } else {
            std::cerr << "Project load failed: " << LayerSerializer::getLastError() << std::endl;
            uiManager_->showExportError(("Load failed: " + LayerSerializer::getLastError()).c_str());
        }
    }

    void saveProject() {
        // Save file dialog
        std::string filename;

#ifdef _WIN32
        // Windows native file dialog
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Ymirge Projects *.ymlayers All Files *.* ";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrDefExt = "ymlayers";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE) {
            filename = ofn.lpstrFile;

            // Ensure .ymlayers extension
            if (filename.find(".ymlayers") == std::string::npos) {
                filename += ".ymlayers";
            }
        } else {
            // User cancelled
            return;
        }
#else
        // On non-Windows, use hardcoded filename for now
        filename = "project.ymlayers";
#endif

        std::cout << "Attempting to save project to: " << filename << std::endl;

        // Save project
        if (LayerSerializer::save(*layerStack_, filename)) {
            std::cout << "Project saved successfully!" << std::endl;
            uiManager_->showExportSuccess("Project saved successfully!");
        } else {
            std::cerr << "Project save failed: " << LayerSerializer::getLastError() << std::endl;
            uiManager_->showExportError(("Save failed: " + LayerSerializer::getLastError()).c_str());
        }
    }

    SDL_Window* window_;
    SDL_GLContext glContext_;
    int screenWidth_;
    int screenHeight_;
    bool running_;

    std::unique_ptr<UIManagerImGui> uiManager_;
    std::unique_ptr<TerrainRendererGL> renderer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<ResolutionManager> resolutionManager_;
    std::unique_ptr<UndoStack> undoStack_;
    std::unique_ptr<LayerUndoStack> layerUndoStack_;
    std::unique_ptr<BrushManager> brushManager_;
    std::unique_ptr<StampTool> stampTool_;
    std::unique_ptr<LayerStack> layerStack_;
    HeightMap compositeHeightMap_;  // Result of compositing all layers

    bool lastUpdateWasGenerating_;
};

int main(int argc, char* argv[]) {
    try {
        YmirgeSDLApp app;
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

#else

#include <iostream>

int main() {
    std::cerr << "This application requires SDL2 UI support. "
              << "Rebuild with -DYMIRGE_BUILD_SDL_UI=ON" << std::endl;
    return 1;
}

#endif // YMIRGE_SDL_UI_ENABLED
