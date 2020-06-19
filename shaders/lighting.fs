# version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureLight;
uniform sampler2D textureDirect;
uniform sampler2D textureIndirect;

// uniform sampler2D shadow;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 lightAmbient;
uniform vec3 lightDiffuse;
uniform vec3 lightSpecular;

uniform int outputType;


float blurLight()
{
    vec2 texelSize = 1.0 / vec2(textureSize(textureLight, 0));
    float result = 0.0;
    for (int x = -3; x < 3; ++x)
    {
        for (int y = -3; y < 3; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(textureLight, texCoord + offset).r;
        }
    }
    return result / (7.0 * 7.0);
}
vec3 blurIndirect()
{
    vec2 texelSize = 1.0 / vec2(textureSize(textureIndirect, 0));
    vec3 result = vec3(0.0);
    for (int x = -3; x < 3; ++x)
    {
        for (int y = -3; y < 3; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(textureIndirect, texCoord + offset).rgb;
        }
    }
    return result / (7.0 * 7.0);
}

void main()
{
    vec3 fragPos   = texture(texturePosition, texCoord).xyz;
    vec3 norm    = texture(textureNormal, texCoord).rgb;
    vec4 color = vec4(texture(textureAlbedo, texCoord).rgb, 1.0);
    float shininess = texture(textureAlbedo, texCoord).a * 10.0;
    float AO = texture(textureDirect, texCoord).r;
    vec4 bounce = vec4(blurIndirect(), 1.0);
    float lighted = blurLight();

    vec4 ambient = vec4(lightAmbient, 1.0) * color * AO;

    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * color * vec4(lightDiffuse, 1.0);

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
    vec4 specular = spec * color * vec4(lightSpecular, shininess);

    if (outputType == 0)
        fragColor = ambient + (diffuse + specular) * lighted + bounce;
    else if (outputType == 1)
        fragColor = ambient + (diffuse + specular) * lighted;
    else if (outputType == 2)
        fragColor = bounce;
    else
        fragColor = vec4(vec3(AO), 1.0);
    // fragColor = vec4(vec3(texture(shadow, texCoord).r), 1.0);
}