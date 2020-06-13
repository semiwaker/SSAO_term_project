# version 450 core

layout (location=0) in vec3 position;
layout (location=1) in vec3 inNormal;
layout (location=2) in vec2 inTexCoord;
layout (location=3) in vec3 inTangent;
layout (location=4) in vec3 inBitangent;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out mat3 TBN;

uniform mat4 modelMat;
uniform mat4 WVP;
uniform mat4 WV;

void main()
{
    fragPos = (WV *  vec4(position, 1.0)).xyz;
    normal = inNormal;
    texCoord = inTexCoord;

    vec3 T   = normalize(mat3(modelMat) * inTangent);
    vec3 B   = normalize(mat3(modelMat) * inBitangent);
    vec3 N   = normalize(mat3(modelMat) * inNormal);
    TBN = mat3(T, B, N);

    gl_Position = WVP * vec4(position, 1.0);
}