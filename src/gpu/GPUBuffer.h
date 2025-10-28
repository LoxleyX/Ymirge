#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include <glad/glad.h>
#include <cstddef>

// Wrapper for OpenGL Shader Storage Buffer Objects (SSBO)
class GPUBuffer {
public:
    // Create buffer with optional initial data
    GPUBuffer(size_t size, const void* data = nullptr);
    ~GPUBuffer();

    // Disable copy
    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    // Enable move
    GPUBuffer(GPUBuffer&& other) noexcept;
    GPUBuffer& operator=(GPUBuffer&& other) noexcept;

    // Upload data to GPU
    void upload(const void* data, size_t size);

    // Download data from GPU
    void download(void* data, size_t size) const;

    // Bind to binding point for use in compute shader
    void bind(GLuint binding) const;

    // Unbind from binding point
    void unbind(GLuint binding) const;

    size_t getSize() const { return size_; }
    GLuint getHandle() const { return buffer_; }

private:
    GLuint buffer_ = 0;
    size_t size_ = 0;
};

#endif // YMIRGE_SDL_UI_ENABLED
