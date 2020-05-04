# version 450 core

layout (location=0) in vec3 position;
layout (location=1) in vec3 inNormal;
layout (location=2) in vec2 inTexCoord;
layout (location=3) in vec3 inTangent;
layout (location=4) in vec3 inBitangent;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out vec3 tangentFragPos;
out vec3 tangentLightPos;
out vec3 tangentViewPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 modelMat;
uniform mat4 VPMat;

void main()
{
    fragPos = vec3(modelMat *  vec4(position, 1.0));
    normal = inNormal;
    texCoord = inTexCoord;

    vec3 T   = normalize(mat3(modelMat) * inTangent);
    vec3 B   = normalize(mat3(modelMat) * inBitangent);
    vec3 N   = normalize(mat3(modelMat) * inNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    tangentLightPos = TBN * lightPos;
    tangentViewPos  = TBN * viewPos;
    tangentFragPos  = TBN * fragPos;

    gl_Position = VPMat * vec4(fragPos, 1.0);
}