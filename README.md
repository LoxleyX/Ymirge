# Ymirge - Procedural Terrain Generator

A real-time procedural terrain generator built with **C++17**, **OpenGL**, and **SDL2**. Features GPU-accelerated noise generation, multi-threaded terrain processing, and an interactive 3D editor with brush tools and layer compositing.

![Status](https://img.shields.io/badge/status-production-brightgreen) ![Version](https://img.shields.io/badge/version-1.1-blue) ![License](https://img.shields.io/badge/license-MIT-blue)

## Tech Stack

- **C++17** with modern STL (smart pointers, move semantics, templates)
- **OpenGL 4.3** compute shaders for GPU-accelerated terrain generation
- **SDL2** for windowing and input handling
- **ImGui** for immediate-mode UI with docking support
- **GLSL** compute shaders for Perlin noise, Gaussian blur, and terrain algorithms
- **Multi-threading** with custom thread pool and lock-free compositing
- **SIMD optimizations** (SSE/AVX when available)

## Features

### Interactive 3D Editor
- **Real-time 3D viewport** with OpenGL rendering and Phong lighting
- **Orbital camera** controls (left-drag to rotate, right-drag to pan, scroll to zoom)
- **60 FPS interface** with smooth, responsive controls
- **Adjustable sea level** with semi-transparent water plane visualization
- **Monochrome mode** toggle for heightmap analysis

### Sculpting Tools
- **5 brush types**: Raise, Lower, Smooth, Flatten, Stamp
- **Adjustable brush size** (1-50 pixels) and strength (0-100%)
- **Stamp tool** with procedural stamp library (mountains, craters, plateaus)
- **Real-time brush cursor** showing affected area on terrain
- **Continuous stroke** support for natural sculpting

### Layer System
- **Photoshop-style layers** with non-destructive compositing
- **8 blend modes**: Normal, Add, Subtract, Multiply, Screen, Max, Min, Overlay
- **Per-layer opacity** control (0-100%)
- **Layer groups** for organization
- **Drag-and-drop** reordering with visual feedback
- **Layer locking** to prevent accidental edits
- **Layer visibility** toggle
- **Mask editing** for selective layer application

### Undo/Redo System
- **Multi-level history** (50 operations)
- **Delta-based storage** (only stores changed pixels, 95% memory reduction)
- **Separate undo stacks** for terrain edits and layer operations
- **Keyboard shortcuts** (Ctrl+Z / Ctrl+Y)

### Procedural Generation
- **GPU-accelerated Perlin noise** (20-30x faster than CPU)
- **8 curated presets**: Game World, Mountain Fortress, Plains, Tropical Island, Alpine Peaks, Canyon Lands, Archipelago
- **19+ adjustable parameters**: Scale, peaks, erosion, rivers, valleys, island shape, sea level
- **Multi-resolution pipeline**: 128×128 to 4096×4096
- **Real-time preview** mode (regenerates at 512×512 while dragging sliders)
- **Async generation** with non-blocking UI updates

### Advanced Algorithms
- **Valley flattening** (3-pass system for ultra-flat gameplay areas)
- **Hydraulic erosion** (particle-based sediment transport)
- **Thermal erosion** (slope-based material sliding)
- **River carving** (A* pathfinding from edges to valleys)
- **Peak generation** (ridged noise for sharp mountains)
- **Edge smoothing** (triple smoothstep for natural coastlines)
- **Valley connectivity** (automatic path connections between flat areas)

### Export Formats
- **PNG16** (16-bit grayscale heightmaps for all game engines)
- **RAW16** (raw 16-bit format for Unity/Unreal direct import)
- **OBJ** (3D mesh with vertex normals and UVs)
- **Splatmap** (RGBA texture with slope-aware material assignment)
  - Red channel: Sand/beach (low elevation, gentle slopes)
  - Green channel: Grass (mid elevation, moderate slopes)
  - Blue channel: Rock (high elevation or steep slopes)
  - Alpha channel: Snow (peak elevations)

### Project Management
- **Save/Load projects** (.ymlayers format with JSON serialization)
- **Layer stack preservation** (saves all layers, blend modes, opacity)
- **Import heightmaps** (PNG 8-bit or 16-bit)
- **Timestamped exports** (automatic filename generation)
- **Native file dialogs** (Windows, Linux, macOS)

## Quick Start

```bash
# Clone repository
git clone https://github.com/loxleyxi/ymirge.git
cd ymirge

# Build (dependencies auto-download)
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64  # Windows
cmake ..                                     # Linux/macOS
cmake --build . --config Release

# Run
./bin/Release/ymirge-ui
```

## Architecture

### Core Systems (`src/core/`)
- **HeightMap** - Float array with SIMD-optimized operations
- **PerlinNoise** - Octave noise with seed control
- **ThreadPool** - Work-stealing parallel execution
- **ResolutionManager** - Multi-res generation with auto-upgrade
- **UndoStack** - Command pattern with delta compression

### Algorithms (`src/algorithms/`)
- **ValleyFlattening** - 3-pass system for ultra-flat gameplay areas
- **EdgeSmoothing** - Triple smoothstep for natural coastlines
- **Peaks** - Ridged noise for sharp mountains
- **Rivers** - A* pathfinding from edges to valleys
- **HydraulicErosion** - Particle-based sediment transport
- **ThermalErosion** - Slope-based material sliding

### GPU Compute (`src/gpu/`)
- **GPUCompute** - OpenGL compute shader abstraction
- **PerlinNoiseGPU** - Parallel noise generation (512×512 in ~10ms)
- **GaussianBlurGPU** - Separable 2D blur for smoothing
- **ComputeShader** - GLSL shader compilation and dispatch

### Rendering (`src/rendering/`)
- **TerrainRendererGL** - Mesh generation with LOD
- **Camera3D** - Orbital camera with smooth interpolation
- **Shader** - Vertex/fragment shader pipeline
- Custom Phong lighting with directional shadows

### UI (`src/ui/`)
- **UIManagerImGui** - Panel layout and event routing
- **PresetManager** - 8 curated terrain configurations
- **StampLibrary** - Procedural and image-based stamps

### Layers (`src/layers/`)
- **LayerStack** - Photoshop-style compositing
- **TerrainLayer** - Height data with blend mode and opacity
- **LayerGroup** - Nested layer organization
- **LayerSerializer** - JSON-based project save/load

## Game Engine Export

### Godot 4.x
```gdscript
# Import heightmap
var image = Image.load_from_file("ymirge_heightmap.png")
var texture = ImageTexture.create_from_image(image)

# Apply to terrain (Terrain3D plugin)
terrain.height_map = texture
terrain.height_scale = 100.0  # Max elevation

# Splatmap shader
shader_type spatial;
uniform sampler2D splatmap;
uniform sampler2D tex_sand : source_color;
uniform sampler2D tex_grass : source_color;
uniform sampler2D tex_rock : source_color;
uniform sampler2D tex_snow : source_color;

void fragment() {
    vec4 splat = texture(splatmap, UV);
    ALBEDO = texture(tex_sand, UV * 10.0).rgb * splat.r
           + texture(tex_grass, UV * 10.0).rgb * splat.g
           + texture(tex_rock, UV * 10.0).rgb * splat.b
           + texture(tex_snow, UV * 10.0).rgb * splat.a;
}
```

### Unity
```csharp
// Import 16-bit PNG as heightmap
// Terrain → Import Heightmap → Select ymirge_heightmap.png
// Terrain Settings:
//   - Depth: 16-bit
//   - Width/Length: Auto-detect
//   - Height: 600 (adjust as needed)

// Splatmap import (requires script)
Texture2D splatmap = Resources.Load<Texture2D>("ymirge_splatmap");
float[,,] alphamaps = new float[resolution, resolution, 4];
// Split RGBA channels into Unity's alphamap format
```

### Unreal Engine
```cpp
// Landscape → Import from File
// Select ymirge_heightmap.png (16-bit)
// Scale: Z = 100.0 (1 unit = 1cm, so 100cm = 1m max height)

// Material: Landscape Layer Blend
// 4 Layers: Sand (R), Grass (G), Rock (B), Snow (A)
// Weight Source: Texture (ymirge_splatmap.png)
```

## Building from Source

### Requirements
- **CMake 3.15+**
- **C++17 compiler**: MSVC 2019+, GCC 9+, or Clang 10+
- **OpenGL 4.3+** support (for compute shaders)

### Dependencies (auto-downloaded)
- SDL2 2.28+
- ImGui 1.89+
- glad (OpenGL loader)
- stb_image, stb_image_write

### Build Options
```bash
# Standard build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Disable GPU compute (CPU-only)
cmake .. -DYMIRGE_ENABLE_GPU=OFF

# Disable UI (console test suite only)
cmake .. -DYMIRGE_BUILD_SDL_UI=OFF
```

### Windows (Visual Studio)
```bash
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux
```bash
# Install OpenGL dev packages
sudo apt install libgl1-mesa-dev libglu1-mesa-dev

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### macOS
```bash
# Xcode Command Line Tools required
xcode-select --install

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## Project Structure

```
ymirge/
├── src/
│   ├── core/              # Foundation (HeightMap, ThreadPool, UndoStack)
│   ├── algorithms/        # Terrain algorithms (valleys, erosion, rivers)
│   ├── gpu/               # OpenGL compute shaders
│   ├── rendering/         # OpenGL mesh rendering
│   ├── ui/                # ImGui interface
│   ├── layers/            # Layer compositing system
│   ├── tools/             # Brush and stamp tools
│   ├── export/            # PNG/RAW/OBJ export
│   └── main_ui_sdl.cpp   # Application entry point
├── shaders/               # GLSL compute shaders
├── docs/                  # Architecture documentation
├── CMakeLists.txt
└── README.md
```

## Key Algorithms

### Valley Flattening (3-Pass System)
1. **Detection**: Find local minima in 10-pixel radius
2. **Extreme Flattening**: 85-100% toward valley floor based on depth
3. **Edge Smoothing**: Gaussian blur in 20-pixel transition zones (4 rounds)

### River Generation
- A* pathfinding from map edges to lowest valleys
- Consistent width with erosion-based carving
- Sediment deposition in flat areas

### Hydraulic Erosion
- Particle-based simulation with water flow
- Sediment capacity based on velocity and slope
- Deposition in low-flow areas (lakes, valleys)

## Technical Highlights

- **Lock-free compositing**: Layers blend without mutex contention
- **SIMD heightmap ops**: 4× faster with SSE2 batch processing
- **Delta-based undo**: Store only changed pixels (95% memory reduction)
- **Async generation**: Worker thread with progress callbacks
- **Shader hot-reload**: Recompile GLSL without restart
- **Custom memory allocators**: Reduced heap fragmentation for large maps
- **Incremental mesh updates**: Only rebuild changed terrain regions

## Performance Optimization Notes

- **Bottleneck**: Valley flattening Pass 3 (4 rounds of 20-pixel Gaussian blur)
- **Solution**: GPU compute shader reduces 2048×2048 to ~400ms (vs 30s CPU)
- **Memory**: Delta undo uses ~5MB for 50 edits (vs 50MB snapshots)
- **Rendering**: 60 FPS maintained with 256×256 mesh (65K triangles)

## License

MIT License - Free for personal and commercial use.

## Credits

- **Tech**: C++17, OpenGL 4.3, SDL2, ImGui, GLSL
- **Libraries**: raylib (prototype), stb (image I/O)
- **Algorithms**: Based on GPU Gems and Procedural Generation research
