#version 330 core

// Inputs from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

// Output
out vec4 FragColor;

// Uniforms
uniform vec3 lightDir;      // Direction to light (normalized)
uniform vec3 lightColor;     // Light color
uniform vec3 viewPos;        // Camera position
uniform float ambientStrength;  // Ambient light strength

void main() {
    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightDir);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.3 * spec * lightColor;

    // Combine lighting with vertex color
    vec3 result = (ambient + diffuse + specular) * Color.rgb;
    FragColor = vec4(result, Color.a);
}
