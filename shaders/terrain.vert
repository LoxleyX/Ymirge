#version 330 core

// Vertex attributes
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

// Outputs to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Transform position to world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform normal to world space (use normal matrix for non-uniform scaling)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass color to fragment shader
    Color = aColor;

    // Transform to clip space
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
