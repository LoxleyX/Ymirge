#ifdef YMIRGE_SDL_UI_ENABLED

#include "ComputeShader.h"
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader(const std::string& shaderPath) {
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << shaderPath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    compileFromSource(buffer.str());
}

ComputeShader ComputeShader::fromSource(const std::string& source) {
    ComputeShader shader;
    shader.compileFromSource(source);
    return shader;
}

ComputeShader::~ComputeShader() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    : program_(other.program_)
    , uniformCache_(std::move(other.uniformCache_)) {
    other.program_ = 0;
}

ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
    if (this != &other) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        program_ = other.program_;
        uniformCache_ = std::move(other.uniformCache_);
        other.program_ = 0;
    }
    return *this;
}

bool ComputeShader::compileFromSource(const std::string& source) {
    const char* src = source.c_str();
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Compute shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return false;
    }

    // Create program
    program_ = glCreateProgram();
    glAttachShader(program_, shader);
    glLinkProgram(program_);

    // Check linking
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program_, 1024, nullptr, infoLog);
        std::cerr << "Compute shader linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(program_);
        program_ = 0;
        glDeleteShader(shader);
        return false;
    }

    glDeleteShader(shader);
    return true;
}

void ComputeShader::bind() const {
    glUseProgram(program_);
}

GLint ComputeShader::getUniformLocation(const std::string& name) {
    auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(program_, name.c_str());
    uniformCache_[name] = location;
    return location;
}

void ComputeShader::setUniform(const std::string& name, int value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform1i(loc, value);
    }
}

void ComputeShader::setUniform(const std::string& name, float value) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform1f(loc, value);
    }
}

void ComputeShader::setUniform(const std::string& name, float x, float y) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform2f(loc, x, y);
    }
}

void ComputeShader::setUniform(const std::string& name, float x, float y, float z) {
    GLint loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform3f(loc, x, y, z);
    }
}

void ComputeShader::dispatch(GLuint groupsX, GLuint groupsY, GLuint groupsZ) {
    glDispatchCompute(groupsX, groupsY, groupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

#endif // YMIRGE_SDL_UI_ENABLED
