#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 InColour;
out vec4 Colour;

uniform mat4 gWVP;

void main()
{
    gl_Position = gWVP * vec4(Position,1.0f);
    Colour = vec4(InColour,1.0f);
}