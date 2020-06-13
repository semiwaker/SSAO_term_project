# version 450 core

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in mat3 TBN;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outAlbedo;

uniform sampler2D textureDiffuse;
uniform sampler2D textureNormals;

uniform float shininess;

void main()
{
    outPosition = vec4(fragPos, 1.0);
    outNormal = normalize(TBN * (texture(textureNormals, texCoord).rgb * 2.0 - 1.0));
    // outNormal = normal;
    outAlbedo = vec4(texture(textureDiffuse, texCoord).rgb, shininess / 10.0);
}