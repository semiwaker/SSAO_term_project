# version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform sampler2D textureAlbedo;
uniform sampler2D textureLight;
uniform sampler2D textureNoise;

uniform vec3 kernel[64];
uniform mat4 projMat;

const vec2 noiseScale = vec2(1600.0, 900.0) / 4.0;

const float radius = 0.1;
const float bias = 0.000;
const float area = 10;

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
        // float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        vec3 Snorm = texture(textureNormal, offset.xy).xyz;
        vec3 SR = normalize(fragPos-s);
        float thetaS = dot(Snorm, SR);
        if (sampleDepth < s.z + bias && abs(sampleDepth-s.z) < 0.5 && thetaS > 0.0)
        {
            vec3 sampleColor = texture(textureAlbedo, offset.xy).rgb;
            float dis = max(1.0, length(s - fragPos));
            float thetaR = abs(dot(normal, SR));
            occlusion += sampleColor * area * thetaS * thetaR / dis / dis;
        }
    }
    occlusion /= 64.0;
    // occlusion = (occlusion / 64.0);
    fragColor = vec4(occlusion, 1.0);
}