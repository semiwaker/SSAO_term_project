# version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureSSAO;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;

void main()
{
    vec3 fragPos   = texture(texturePosition, texCoord).xyz;
    vec3 norm    = texture(textureNormal, texCoord).rgb;
    vec4 color = vec4(texture(textureAlbedo, texCoord).rgb, 1.0);
    float shininess = texture(textureAlbedo, texCoord).a * 10.0;
    float AO = texture(textureSSAO, texCoord).r;

    vec4 ambient = vec4(lightAmbient, 1.0) * color * AO;

    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * color * vec4(lightDiffuse, 1.0);

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
    vec4 specular = spec * color * vec4(lightSpecular, shininess);

    fragColor = ambient + diffuse; //+ specular;
}