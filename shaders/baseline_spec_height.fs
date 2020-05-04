#version 450 core

out vec4 FragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec3 tangentFragPos;
in vec3 tangentLightPos;
in vec3 tangentViewPos;

uniform mat4 modelMat;

uniform sampler2D textureDiffuse;
uniform sampler2D textureSpecular;
uniform sampler2D textureHeight;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;

vec2 ParallaxMapping(vec3 viewDir)
{
    float height = texture(textureHeight, texCoord).r;
    vec2 p = viewDir.xy / viewDir.z * (height * 0.1);
    return texCoord - p;
}

void main()
{
    vec2 newTexCoord = ParallaxMapping(normalize(tangentViewPos - tangentFragPos));

    vec4 ambient = vec4(lightAmbient, 1.0) * texture(textureDiffuse, newTexCoord);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * texture(textureDiffuse, newTexCoord) * vec4(lightDiffuse, 1.0);

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
    vec4 specular = spec * texture(textureSpecular, newTexCoord) * vec4(lightSpecular, 1.0);

    FragColor = ambient + diffuse + specular;
}
