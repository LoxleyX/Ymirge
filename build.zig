const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Main executable
    const exe = b.addExecutable(.{
        .name = "ymirge",
        .target = target,
        .optimize = optimize,
    });

    // C++ compilation flags
    const cpp_flags = &[_][]const u8{
        "-std=c++17",
        "-DYMIRGE_SDL_UI_ENABLED",
        "-DIMGUI_IMPL_OPENGL_LOADER_GLAD",
    };

    // ImGui-specific flags (use our custom config to disable SSE)
    const imgui_flags = &[_][]const u8{
        "-std=c++17",
        "-DIMGUI_IMPL_OPENGL_LOADER_GLAD",
        "-DIMGUI_USER_CONFIG=\"ymirge_imconfig.h\"",
    };

    const release_flags = if (optimize != .Debug)
        &[_][]const u8{ "-O2", "-DNDEBUG" }
    else
        &[_][]const u8{ "-g", "-O0" };

    // Include paths
    exe.addIncludePath(.{ .cwd_relative = "src" });
    exe.addIncludePath(.{ .cwd_relative = "src/core" });
    exe.addIncludePath(.{ .cwd_relative = "src/algorithms" });
    exe.addIncludePath(.{ .cwd_relative = "src/gpu" });
    exe.addIncludePath(.{ .cwd_relative = "src/rendering" });
    exe.addIncludePath(.{ .cwd_relative = "src/layers" });
    exe.addIncludePath(.{ .cwd_relative = "src/tools" });
    exe.addIncludePath(.{ .cwd_relative = "src/ui" });
    exe.addIncludePath(.{ .cwd_relative = "src/export" });
    exe.addIncludePath(.{ .cwd_relative = "vendor" });
    exe.addIncludePath(.{ .cwd_relative = "vendor/glad/include" });
    exe.addIncludePath(.{ .cwd_relative = "vendor/imgui" });
    exe.addIncludePath(.{ .cwd_relative = "vendor/imgui/backends" });
    exe.addIncludePath(.{ .cwd_relative = "vendor/glm" });


    // Source files
    const cpp_sources = &[_][]const u8{
        // Core
        "src/core/HeightMap.cpp",
        "src/core/HeightMapEditCommand.cpp",
        "src/core/PerlinNoise.cpp",
        "src/core/ResolutionManager.cpp",
        "src/core/TerrainGenerator.cpp",
        "src/core/ThreadPool.cpp",
        "src/core/UndoStack.cpp",

        // Algorithms
        "src/algorithms/EdgeSmoothing.cpp",
        "src/algorithms/HydraulicErosion.cpp",
        "src/algorithms/Peaks.cpp",
        "src/algorithms/RiverEnhancements.cpp",
        "src/algorithms/Rivers.cpp",
        "src/algorithms/TerrainSoftening.cpp",
        "src/algorithms/ThermalErosion.cpp",
        "src/algorithms/ValleyConnectivity.cpp",
        "src/algorithms/ValleyFlattening.cpp",

        // GPU
        "src/gpu/ComputeShader.cpp",
        "src/gpu/GaussianBlurGPU.cpp",
        "src/gpu/GPUBuffer.cpp",
        "src/gpu/GPUCompute.cpp",
        "src/gpu/GPUTest.cpp",
        "src/gpu/PerlinNoiseGPU.cpp",

        // Rendering
        "src/rendering/Camera3D.cpp",
        "src/rendering/Shader.cpp",
        "src/rendering/TerrainRendererGL.cpp",

        // Layers
        "src/layers/LayerBase.cpp",
        "src/layers/LayerCommand.cpp",
        "src/layers/LayerGroup.cpp",
        "src/layers/LayerSerializer.cpp",
        "src/layers/LayerStack.cpp",
        "src/layers/LayerThumbnail.cpp",
        "src/layers/TerrainLayer.cpp",

        // Tools
        "src/tools/BrushManager.cpp",
        "src/tools/PathTool.cpp",
        "src/tools/PolygonSelection.cpp",
        "src/tools/StampTool.cpp",

        // UI
        "src/ui/PresetManager.cpp",
        "src/ui/UIManagerImGui.cpp",

        // Export
        "src/export/AdvancedSplatmap.cpp",
        "src/export/ImageExporter.cpp",

        // Main
        "src/main_ui_sdl.cpp",
    };

    // ImGui sources (vendored)
    const imgui_sources = &[_][]const u8{
        "vendor/imgui/imgui.cpp",
        "vendor/imgui/imgui_demo.cpp",
        "vendor/imgui/imgui_draw.cpp",
        "vendor/imgui/imgui_tables.cpp",
        "vendor/imgui/imgui_widgets.cpp",
        "vendor/imgui/backends/imgui_impl_sdl2.cpp",
        "vendor/imgui/backends/imgui_impl_opengl3.cpp",
    };

    // glad source
    const glad_sources = &[_][]const u8{
        "vendor/glad/src/glad.c",
    };

    // Add C++ sources
    for (cpp_sources) |src| {
        exe.addCSourceFile(.{
            .file = .{ .cwd_relative = src },
            .flags = cpp_flags ++ release_flags,
        });
    }

    // Add ImGui sources (with SSE disabled)
    for (imgui_sources) |src| {
        exe.addCSourceFile(.{
            .file = .{ .cwd_relative = src },
            .flags = imgui_flags ++ release_flags,
        });
    }

    // Add glad (C source)
    for (glad_sources) |src| {
        exe.addCSourceFile(.{
            .file = .{ .cwd_relative = src },
            .flags = &[_][]const u8{},
        });
    }

    // Link system libraries
    exe.linkLibC();
    exe.linkLibCpp();
    exe.linkSystemLibrary("SDL2");
    exe.linkSystemLibrary("GL");
    exe.linkSystemLibrary("dl");
    exe.linkSystemLibrary("pthread");

    // Install executable to prefix root (not prefix/bin)
    b.getInstallStep().dependOn(&b.addInstallArtifact(exe, .{
        .dest_dir = .{ .override = .prefix },
    }).step);

    // Install shaders directory alongside executable
    b.installDirectory(.{
        .source_dir = .{ .cwd_relative = "shaders" },
        .install_dir = .prefix,
        .install_subdir = "shaders",
    });

    // Install assets directory alongside executable
    b.installDirectory(.{
        .source_dir = .{ .cwd_relative = "assets" },
        .install_dir = .prefix,
        .install_subdir = "assets",
    });

    // Run step
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    // Run from the install directory so shaders are found
    run_cmd.setCwd(.{ .cwd_relative = "bin" });

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run Ymirge");
    run_step.dependOn(&run_cmd.step);
}
