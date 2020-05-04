#version 450 core

out vec4 FragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec3 tangentFragPos;
in vec3 tangentLightPos;
in vec3 tangentViewPos;

uniform sampler2D textureDiffuse;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;

void main()
{
    vec4 ambient = vec4(lightAmbient, 1.0) * texture(textureDiffuse, texCoord);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * texture(textureDiffuse, texCoord) * vec4(lightDiffuse, 1.0);

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = max(dot(viewDir, reflectDir), 0.0);
    vec4 specular = spec * vec4(1.0f, 1.0f, 1.0f, 1.0f) * vec4(lightSpecular, 1.0);

    FragColor = ambient + diffuse + specular;
}
