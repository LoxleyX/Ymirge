#version 330 core

// Inputs from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

// Outputs
out vec4 FragColor;

// Uniforms
uniform vec3 lightDir;      // Directional light direction (normalized)
uniform vec3 viewPos;       // Camera position
uniform vec3 lightColor;    // Light color
uniform float ambientStrength;  // Ambient light strength

void main() {
    // Normalize interpolated normal
    vec3 norm = normalize(Normal);

    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting (Lambertian)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * lightColor * 0.3;  // 30% specular strength

    // Combine lighting with vertex color
    vec3 result = (ambient + diffuse + specular) * Color.rgb;

    FragColor = vec4(result, Color.a);
}
