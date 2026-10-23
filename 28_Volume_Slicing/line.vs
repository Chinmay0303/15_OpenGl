#version 330 core

layout (location = 0) in vec3 Position;

uniform mat4 gWorld;
uniform mat4 gView;
uniform mat4 gProjection;

void main()
{
    gl_Position = gProjection * gView * gWorld * vec4(Position, 1.0);
}

