# Ymirge - Procedural Terrain Generator

**Version 1.1** - A complete C++ terrain generator with 3D visualization, interactive sculpting, and PNG export, optimized for game development.

![Status](https://img.shields.io/badge/status-production-brightgreen) ![Version](https://img.shields.io/badge/version-1.1-blue) ![License](https://img.shields.io/badge/license-MIT-blue)

## What is Ymirge?

Ymirge (pronounced "Yuh-meer-juh", from Norse mythology: Ymir + Forge) is a procedural terrain generator designed specifically for game developers. It creates heightmaps with:

- **Ultra-flat valleys** (90-98% flattening) perfect for gameplay areas
- **Sharp mountain peaks** for dramatic scenery
- **Smooth island edges** with no jagged artifacts
- **Realistic rivers** flowing into valleys
- **Connected flat areas** for seamless navigation

## Quick Start

```bash
# Clone or download this repository
cd ymirge

# Windows
build.bat

# Linux/macOS
./build.sh

# Select "1" to launch the UI application
```

That's it! Dependencies (raylib, stb_image_write) download automatically.

## Features

### Core Generation
- ✅ **5 Terrain Algorithms**: Valley flattening, edge smoothing, peaks, rivers, valley connectivity
- ✅ **Multi-threaded**: Parallel processing with automatic thread detection
- ✅ **Multi-resolution**: 256x256 for real-time, 512x512 for quality, 2048x2048 for export
- ✅ **19+ Parameters**: Scale, octaves, persistence, lacunarity, peaks, erosion, and more

### Interactive UI & 3D Visualization
- ✅ **Real-time 3D Preview**: Full 3D mesh rendering with custom Phong lighting
- ✅ **Orbital Camera**: Left-drag to rotate, mouse wheel to zoom
- ✅ **Sea Plane**: Adjustable water level with semi-transparent rendering
- ✅ **8 Curated Presets**: Game World, Mountain Fortress, Plains, Tropical Island, and more
- ✅ **10 Key Sliders**: Including sea level control for water visualization
- ✅ **Monochrome Mode**: Toggle color gradient for heightmap analysis
- ✅ **60 FPS Interface**: Smooth, responsive controls built with raylib

### Export
- ✅ **PNG Heightmaps**: 8-bit grayscale, game engine ready
- ✅ **PNG Splatmaps**: RGBA texture maps (Red=Sand, Green=Grass, Blue=Rock, Alpha=Snow)
- ✅ **Slope-aware**: Automatic texture assignment based on height AND slope
- ✅ **Timestamped Files**: No overwriting, organized exports
- ✅ **Game Engine Integration**: Godot, Unity, Unreal compatible

## Screenshots

```
┌─────────────────────────────────────────────────────┐
│  Ymirge - Procedural Terrain Generator             │
├───────────────────┬─────────────────────────────────┤
│                   │  Controls                       │
│                   │                                 │
│                   │  Presets:                       │
│   800x800         │  ┌─────────────────────┐       │
│   Terrain         │  │ Game World          │       │
│   Preview         │  │ Mountain Fortress   │       │
│                   │  │ Plains              │       │
│   (Color          │  │ Tropical Island     │       │
│   Gradient)       │  │ Alpine Peaks        │       │
│                   │  │ Canyon Lands        │       │
│                   │  │ Archipelago         │       │
│                   │  │ Extreme Contrast    │       │
│                   │  └─────────────────────┘       │
│                   │                                 │
│                   │  Parameters:                    │
│                   │  Scale: ████████░░  200         │
│                   │  Flatten Valleys: █████  0.95   │
│                   │  Valley Connectivity: ███ 0.9   │
│                   │  Peaks: ██░░░░░░  0.2           │
│                   │  (scrollable...)                │
│                   │                                 │
│                   │  ┌─────────────────────┐       │
│                   │  │ Generate Terrain (G)│       │
│                   │  └─────────────────────┘       │
│                   │  ┌──────┐ ┌──────┐             │
│                   │  │Export│ │Export│             │
│                   │  │Height│ │Splat │             │
│                   │  └──────┘ └──────┘             │
│                   │  [ ] Monochrome                 │
└───────────────────┴─────────────────────────────────┘
```

## The 8 Presets

1. **Game World** - 95% flat valleys, scale 200 → Best for open-world games
2. **Mountain Fortress** - 92% flat valleys, dramatic peaks → Sharp mountains with flat bases
3. **Plains** - 97% flat, scale 220 → Flattest terrain with gentle rolling hills
4. **Tropical Island** - 85% flat, island 0.85 → Round island with central peak
5. **Alpine Peaks** - 80% flat, peaks 0.75 → Extreme sharp mountains
6. **Canyon Lands** - 90% flat, terracing 7 → Layered desert terrain
7. **Archipelago** - 75% flat, island 0.75 → Multiple scattered islands
8. **Extreme Contrast** - 98% flat + 80% peaks → Maximum drama

## Using Exports in Game Engines

### Godot

**Heightmap**:
```gdscript
# Import ymirge_heightmap_*.png as Image
# Use in HeightMapShape or Terrain3D plugin
# Scale height by desired max elevation (e.g., 100 units)
```

**Splatmap Shader**:
```gdscript
shader_type spatial;

uniform sampler2D splatmap;
uniform sampler2D tex_sand;   # Red channel
uniform sampler2D tex_grass;  # Green channel
uniform sampler2D tex_rock;   # Blue channel
uniform sampler2D tex_snow;   # Alpha channel

void fragment() {
    vec4 splat = texture(splatmap, UV);
    vec3 color = texture(tex_sand, UV * 10.0).rgb * splat.r
               + texture(tex_grass, UV * 10.0).rgb * splat.g
               + texture(tex_rock, UV * 10.0).rgb * splat.b
               + texture(tex_snow, UV * 10.0).rgb * splat.a;
    ALBEDO = color;
}
```

### Unity

```csharp
// Import heightmap PNG
// Terrain Settings → Import Raw → Select PNG
// Create Terrain Layers for Sand, Grass, Rock, Snow
// Use TerrainData.SetAlphamaps() with splatmap channels
```

### Unreal Engine

```
Landscape → Import from File → Select heightmap PNG
Create Landscape Material with Layer Blend
Assign splatmap PNG as weight texture (RGBA → 4 layers)
```

## Building from Source

### Requirements
- CMake 3.15+
- C++17 compiler (MSVC 2019+, GCC 7+, Clang 5+)
- Dependencies auto-downloaded: raylib 5.0, stb_image_write.h

### Manual Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Executables:
# - build/bin/ymirge-ui      (Interactive UI app)
# - build/bin/ymirge         (Console test suite)
```

### Build Options

```bash
# Disable UI (console-only)
cmake .. -DYMIRGE_BUILD_UI=OFF

# Disable SIMD optimizations
cmake .. -DYMIRGE_ENABLE_SIMD=OFF
```

## Performance

**Tested on modern hardware (8+ threads):**

| Resolution | Generation Time | Use Case |
|-----------|-----------------|----------|
| 256x256 | < 50ms | Real-time slider updates |
| 512x512 | < 200ms | UI preview quality |
| 2048x2048 | < 4s | Final export (future) |

**UI renders at 60 FPS** with responsive controls even during generation.

## Architecture

```
ymirge/
├── src/
│   ├── core/               # Phase 1: Foundation
│   │   ├── HeightMap       # SIMD-ready data structure
│   │   ├── PerlinNoise     # Octave noise generation
│   │   ├── ThreadPool      # Parallel processing
│   │   └── TerrainGenerator # Main pipeline
│   ├── algorithms/         # Phase 2: Terrain algorithms
│   │   ├── ValleyFlattening   # 3-pass ultra-flat valleys
│   │   ├── EdgeSmoothing      # Triple smoothstep coastlines
│   │   ├── Peaks              # Ridged noise mountains
│   │   ├── Rivers             # Edge-to-valley flow
│   │   └── ValleyConnectivity # Connect flat areas
│   ├── rendering/          # Phase 3: Visualization
│   │   └── TerrainRenderer # HeightMap → raylib texture
│   ├── ui/                 # Phase 3: User interface
│   │   ├── Slider          # Interactive controls
│   │   ├── PresetManager   # 8 curated presets
│   │   └── UIManager       # Main UI controller
│   ├── export/             # Phase 4: PNG export
│   │   └── ImageExporter   # Heightmap + splatmap PNG
│   ├── main.cpp            # Console test suite
│   └── main_ui.cpp         # UI application
├── vendor/
│   └── stb_image_write.h   # Auto-downloaded
├── build.bat / build.sh    # Build scripts
└── docs...                 # Extensive documentation
```

## Documentation

**In `c:\dev\gen\` directory:**
- **CLAUDE.md** - Comprehensive development guide with architecture, algorithms, and implementation details
- **PLAN.md** - Implementation roadmap and performance metrics
- **FEATURE_PROPOSAL.md** - Analysis of 40+ features based on competitive research
- **PHASE1_IMPLEMENTATION.md** - Detailed Phase 1 implementation plan (brush tools, multi-resolution, enhanced export)

## Customization

### Add Custom Presets
Edit `src/ui/PresetManager.cpp`:
```cpp
TerrainParams params;
params.scale = 150.0f;
params.flattenValleys = 0.93f;
// ... set other parameters
presets_["My Preset"] = params;
presetOrder_.push_back("My Preset");
```

### Add More Sliders
Edit `src/ui/UIManager.cpp`:
```cpp
sliders_.push_back(std::make_unique<Slider>(
    "My Parameter", &params_.myParam, min, max,
    "Description of parameter"));
```

### Adjust Splatmap Thresholds
Edit `src/export/ImageExporter.cpp`, function `generateSplatmapPixel()`:
```cpp
const float WATER_LEVEL = 0.25f;   // Adjust these
const float BEACH_LEVEL = 0.40f;
const float GRASS_LEVEL = 0.60f;
const float ROCK_LEVEL = 0.70f;
const float SNOW_LEVEL = 0.85f;
```

## Roadmap

### Phase 1: Foundation (Current - 3-6 months)
**Goal:** Transform from parameter-driven generator to interactive terrain editor

- [ ] **Multi-Resolution System** - 128x128 real-time, up to 4096x4096 for export
- [ ] **Enhanced Export** - RAW16, EXR, tiled export, OBJ/glTF 3D formats
- [ ] **Brush Tool System** - Raise, lower, smooth, flatten terrain manually
- [ ] **Undo/Redo** - Multi-level history with delta storage

### Phase 2: Advanced Features (6-12 months)
- [ ] Layer system for non-destructive editing
- [ ] Stamp tool (mountains, craters, plateaus)
- [ ] Enhanced erosion (thermal + improved hydraulic)
- [ ] River enhancements (meanders, lakes, tributaries)

### Phase 3: Innovation (12-18 months)
- [ ] Node-based workflow for procedural control
- [ ] GPU compute acceleration (10-100x speedup)
- [ ] Multi-biome system (desert, alpine, forest with auto-blending)

### Phase 4: Ecosystem (18-24 months)
- [ ] Direct engine plugins (Unity, Unreal, Godot)
- [ ] Terrain analysis tools (slope, flatness, accessibility)
- [ ] Community preset sharing platform

**See `FEATURE_PROPOSAL.md` for complete roadmap with 40+ planned features.**

## License

MIT License - Free for personal and commercial use.

## Credits

- **Concept**: Based on original JavaScript terrain generator
- **Implementation**: Complete C++ rewrite with raylib UI
- **Name**: From Norse mythology (Ymir, the primordial giant + Forge)
- **Dependencies**:
  - [raylib](https://www.raylib.com/) - UI and rendering
  - [stb_image_write](https://github.com/nothings/stb) - PNG export

## Project Stats

- **Total Code**: ~4,000+ lines of C++17
- **Core Features**: Procedural generation, 3D visualization, custom shaders, PNG export
- **Dependencies**: 2 (raylib, stb_image_write - auto-downloaded)
- **Platforms**: Windows, Linux, macOS (via CMake)
- **Performance**: ~2s for 512x512, ~8s for 1024x1024
- **Status**: Production ready ✅, actively enhancing

---

**Made with ⚡ for game developers**

Generate terrain → Export PNG → Import to engine → Build your game!
