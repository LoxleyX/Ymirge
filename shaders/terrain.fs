#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Input uniform values
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    // Ambient lighting
    vec3 ambientLight = ambient.rgb * ambient.a;

    // Diffuse lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDirection = normalize(lightDir);
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (subtle)
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = spec * lightColor * 0.3;

    // Combine lighting with vertex color
    vec3 lighting = ambientLight + diffuse + specular;
    finalColor = vec4(fragColor.rgb * lighting, fragColor.a);
}
