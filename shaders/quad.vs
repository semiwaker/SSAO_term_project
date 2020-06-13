# version 450 core

layout (location=0) in vec2 inPos;

out vec2 texCoord;

void main()
{
    gl_Position = vec4(inPos, 0.0, 1.0);
    texCoord = inPos * 0.5 + 0.5;
}