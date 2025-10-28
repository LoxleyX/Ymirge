#version 330 core

// Vertex attributes
layout (location = 0) in vec3 aPos;      // Position
layout (location = 1) in vec3 aNormal;   // Normal
layout (location = 2) in vec4 aColor;    // Vertex color (based on height)

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

void main() {
    // Transform position to world space
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Transform normal to world space (need transpose(inverse(model)) for non-uniform scaling)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass color through
    Color = aColor;

    // Transform to clip space
    gl_Position = projection * view * worldPos;
}
