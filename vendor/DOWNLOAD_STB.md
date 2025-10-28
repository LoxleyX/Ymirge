# Download stb_image_write.h

Phase 4 requires the stb_image_write.h header for PNG export.

## Automatic Download (Recommended)

The CMake build system will automatically download this header on first build.

## Manual Download (Optional)

If you prefer to download manually:

1. Visit: https://github.com/nothings/stb
2. Download `stb_image_write.h`
3. Place it in this directory: `vendor/stb_image_write.h`

## About stb_image_write

- **License**: Public Domain
- **Author**: Sean Barrett
- **Size**: ~1500 lines (single header)
- **Formats**: PNG, BMP, TGA, JPG
- **Usage**: `#define STB_IMAGE_WRITE_IMPLEMENTATION` before including

## What We Use It For

- Heightmap export (16-bit grayscale PNG)
- Splatmap export (RGBA PNG)
- Optional: Screenshots of terrain preview

---

**Status**: Will be auto-downloaded by CMake in Phase 4
