#version 450 core
layout (location = 0) in vec3 position;

out vec3 localPos;

uniform mat4 proj;
uniform mat4 view;

void main()
{
    localPos = position;
    gl_Position =  proj * view * vec4(localPos, 1.0);
}