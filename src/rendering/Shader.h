#pragma once

#ifdef YMIRGE_SDL_UI_ENABLED

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

/**
 * Shader Program Helper
 *
 * Handles loading, compiling, and using GLSL shaders.
 * Provides convenient uniform setters.
 */
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Use this shader program
    void use();

    // Uniform setters
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat4(const std::string& name, const glm::mat4& value);

    // Get program ID
    unsigned int getProgramID() const { return programID_; }

private:
    unsigned int programID_;

    // Helper functions
    std::string readFile(const std::string& path);
    unsigned int compileShader(unsigned int type, const std::string& source);
    void checkCompileErrors(unsigned int shader, const std::string& type);
    void checkLinkErrors(unsigned int program);
};

#endif // YMIRGE_SDL_UI_ENABLED
