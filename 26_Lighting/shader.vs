#version 330 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 InColour;
layout(location = 2) in vec3 InNormal;

uniform mat4 gWorld;
uniform mat4 gWVP;

// out vec3 VertexColour;
out vec3 VertexNormal;

void main()
{
    gl_Position = gWVP *  vec4(Position,1.0f);

    // VertexColour = InColour;

    mat3 normalMatrix = transpose(inverse(mat3(gWorld)));

    VertexNormal = normalize( normalMatrix * InNormal);
}