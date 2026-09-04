#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 InColour;
out vec3 VertexColour;

uniform mat4 gWVP;

void main()
{
    gl_Position = gWVP * vec4(Position,1.0f);

    VertexColour = InColour;
}