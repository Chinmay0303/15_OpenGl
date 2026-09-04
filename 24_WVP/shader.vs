#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 InColour;
out vec4 Colour;

uniform mat4 gWVP;

// mat3 Z = mat3(
//             gScale,0,0, // first column
//             0,gScale,0, // second column
//             0,0,1,      // third column
//         );

void main()
{
    gl_Position = gWVP * vec4(Position,1.0f);
    Colour = vec4(InColour,1.0f);
}