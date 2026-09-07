#version 330 core

layout(location = 0) in vec3 Position;
// layout(location = 1) in vec3 InColour;
layout(location = 2) in vec3 InNormal;

uniform mat4 gWorld;
uniform mat4 gClipWorld;
uniform mat4 gView;
uniform mat4 gProjection;

// out vec3 VertexColour;
out vec3 ViewNormal;
out vec3 ViewPosition;
out vec3 ClipWorldPosition;
out vec3 WorldPosition;

out float VertexDepth;

void main()
{

    mat4 gWorldView = gView * gWorld;

    vec4 positionView = gWorldView * vec4(Position, 1.0);
    vec4 clipWorldPosition = gClipWorld * vec4(Position, 1.0);
    vec4 positionWorld = gWorld * vec4(Position, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(gWorldView)));

    ViewNormal = normalize(normalMatrix * InNormal);
    ViewPosition = positionView.xyz;
    ClipWorldPosition = clipWorldPosition.xyz;
    WorldPosition = positionWorld.xyz;

    gl_Position = gProjection * positionView;

    // VertexDepth = length(positionView.xyz);
    VertexDepth = -positionView.z;
}