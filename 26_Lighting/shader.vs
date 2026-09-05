#version 330 core

layout(location = 0) in vec3 Position;
// layout(location = 1) in vec3 InColour;
layout(location = 2) in vec3 InNormal;

uniform mat4 gWorld;
uniform mat4 gWVP;

// out vec3 VertexColour;
out vec3 WorldNormal;
out vec3 WorldPosition;

void main()
{
   vec4 worldPosition = gWorld * vec4(Position, 1.0);

    gl_Position = gWVP * vec4(Position, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(gWorld)));

    WorldPosition = worldPosition.xyz;
    WorldNormal = normalize(normalMatrix * InNormal);
}