# version 450 core

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in mat3 TBN;
in vec4 lightSpacePos;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out float outLight;

uniform sampler2D textureDiffuse;
uniform sampler2D textureNormals;
uniform sampler2D textureShadow;

uniform vec3 lightDir;

uniform float shininess;


void main()
{
    outPosition = vec4(fragPos, 1.0);
    outNormal = normalize(TBN * (texture(textureNormals, texCoord).rgb * 2.0 - 1.0));
    // outNormal = normal;
    outAlbedo = vec4(texture(textureDiffuse, texCoord).rgb, shininess / 10.0);
    vec3 light = lightSpacePos.xyz / lightSpacePos.w;
    vec3 shadowPos = light * 0.5 + 0.5;

    float bias = max(0.0005 * (1.0 - dot(outNormal, normalize(lightDir))), 0.0);
    outLight = shadowPos.z > 1.0 ? 1.0 : step(shadowPos.z, texture(textureShadow, shadowPos.xy).r + bias);
    // outLight = vec4(outPosition.xyz * 0.5 + 0.5, 1.0);
}