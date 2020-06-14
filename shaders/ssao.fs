# version 450 core

in vec2 texCoord;

out float fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureLight;
uniform sampler2D textureNoise;

uniform vec3 kernel[64];
uniform mat4 projMat;

const vec2 noiseScale = vec2(1600.0, 900.0) / 4.0;

const float radius = 0.5;
const float bias = 0.0;
const float occPower = 1.0;

void main()
{
    vec3 fragPos   = texture(texturePosition, texCoord).xyz;
    vec3 normal    = texture(textureNormal, texCoord).rgb;
    vec3 randomVec = texture(textureNoise, texCoord * noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i)
    {
        vec3 s = TBN * kernel[i];
        s = fragPos + s * radius;
        vec4 offset = vec4(s, 1.0);

        offset = projMat * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(texturePosition, offset.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= s.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);
    fragColor = pow(occlusion, occPower);
}