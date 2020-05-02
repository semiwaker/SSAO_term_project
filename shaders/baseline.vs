# version 450 core

layout (location=0) in vec3 position;
layout (location=1) in vec3 inNormal;
layout (location=2) in vec2 inTexCoord;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

uniform mat4 modelMat;
uniform mat4 VPMat;

void main()
{
    fragPos = vec3(modelMat *  vec4(position, 1.0));
    normal = inNormal;
    texCoord = inTexCoord;

    gl_Position = VPMat * vec4(fragPos, 1.0);
}