# version 450 core

layout (location=0) in vec3 position;
layout (location=1) in vec3 inNormal;
layout (location=2) in vec2 inTexCoord;
layout (location=3) in vec3 inTangent;
layout (location=4) in vec3 inBitangent;


uniform mat4 modelMat;
uniform mat4 lightMat;

void main()
{
    gl_Position = lightMat * modelMat * vec4(position, 1.0);
}