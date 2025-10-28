#ifdef YMIRGE_SDL_UI_ENABLED

#include "GPUBuffer.h"
#include <iostream>

GPUBuffer::GPUBuffer(size_t size, const void* data)
    : size_(size) {
    glGenBuffers(1, &buffer_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GPUBuffer::~GPUBuffer() {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
    }
}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
    : buffer_(other.buffer_)
    , size_(other.size_) {
    other.buffer_ = 0;
    other.size_ = 0;
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept {
    if (this != &other) {
        if (buffer_ != 0) {
            glDeleteBuffers(1, &buffer_);
        }
        buffer_ = other.buffer_;
        size_ = other.size_;
        other.buffer_ = 0;
        other.size_ = 0;
    }
    return *this;
}

void GPUBuffer::upload(const void* data, size_t size) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
    if (size <= size_) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    } else {
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY);
        size_ = size;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUBuffer::download(void* data, size_t size) const {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (ptr) {
        memcpy(data, ptr, size);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUBuffer::bind(GLuint binding) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer_);
}

void GPUBuffer::unbind(GLuint binding) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
}

#endif // YMIRGE_SDL_UI_ENABLED
