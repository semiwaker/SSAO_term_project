# version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureLight;
uniform sampler2D textureNoise;
uniform samplerCube textureCubeMap;

uniform vec3 kernel[64];
uniform mat4 viewMat;
uniform mat4 projMat;

uniform int AOType;

const vec2 noiseScale = vec2(1600.0, 900.0) / 4.0;

const float radius = 0.01;
const float bias = 0.000;

vec3 Illuminance(vec3 v)
{
    vec3 dir = inverse(mat3(viewMat)) * -v;
    vec3 hdr = texture(textureCubeMap, dir.xyz).rgb;
    return hdr;
}

void main()
{
    vec3 fragPos   = texture(texturePosition, texCoord).xyz;
    vec3 normal    = texture(textureNormal, texCoord).rgb;
    vec3 randomVec = texture(textureNoise, texCoord * noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    vec3 occlusion = vec3(0.0);
    for(int i = 0; i < 64; ++i)
    {
        vec3 dir = TBN * kernel[i];
        vec3 s = fragPos + dir * radius;
        vec4 offset = vec4(s, 1.0);

        offset = projMat * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(texturePosition, offset.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        if (AOType == 0)
        {
            if (sampleDepth >= s.z + bias)
                occlusion += Illuminance(dir) * dot(normal, normalize(dir)) * rangeCheck * 0.5;
        } else
        if (AOType == 1)
            occlusion += vec3(sampleDepth >= s.z + bias ? 1.0 : 0.0) * rangeCheck;
        else if (AOType == 2)
            occlusion += vec3(1.0);
    }
    occlusion = (occlusion / 64.0);
    fragColor = vec4(occlusion, 1.0);
}