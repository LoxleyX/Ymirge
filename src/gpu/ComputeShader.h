#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include <glad/glad.h>
#include <string>
#include <unordered_map>

// Wrapper for OpenGL compute shaders
class ComputeShader {
public:
    // Load and compile compute shader from file
    explicit ComputeShader(const std::string& shaderPath);

    // Load and compile compute shader from source string
    static ComputeShader fromSource(const std::string& source);

    ~ComputeShader();

    // Disable copy
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;

    // Enable move
    ComputeShader(ComputeShader&& other) noexcept;
    ComputeShader& operator=(ComputeShader&& other) noexcept;

    // Bind shader for use
    void bind() const;

    // Set uniforms
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y);
    void setUniform(const std::string& name, float x, float y, float z);

    // Dispatch compute shader
    // Groups are calculated as: (width + localSizeX - 1) / localSizeX
    void dispatch(GLuint groupsX, GLuint groupsY = 1, GLuint groupsZ = 1);

    // Check if shader compiled successfully
    bool isValid() const { return program_ != 0; }

private:
    ComputeShader() = default;

    bool compileFromSource(const std::string& source);
    GLint getUniformLocation(const std::string& name);

    GLuint program_ = 0;
    std::unordered_map<std::string, GLint> uniformCache_;
};

#endif // YMIRGE_SDL_UI_ENABLED
