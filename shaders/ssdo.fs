# version 450 core

in vec2 texCoord;

out float fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureLight;
uniform sampler2D textureNoise;
uniform sampler2D textureHDR;

uniform vec3 kernel[64];
uniform mat4 viewMat;
uniform mat4 projMat;

uniform int AOType;

const vec2 noiseScale = vec2(1600.0, 900.0) / 4.0;

const float radius = 0.01;
const float bias = 0.000;
const float occPower = 1.0;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec3 v1 = inverse(mat3(viewMat)) * v;
    vec2 uv = vec2(atan(v1.z, v1.x), asin(v1.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
float Illuminance(vec3 v)
{
    float hdr = texture(textureHDR, SampleSphericalMap(v)).a;
    return exp2(hdr - 128);
}

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
                occlusion += (1.0 - Illuminance(dir)) * rangeCheck;
        } else
        if (AOType == 1)
            occlusion += (sampleDepth >= s.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);
    fragColor = pow(occlusion, occPower);
}