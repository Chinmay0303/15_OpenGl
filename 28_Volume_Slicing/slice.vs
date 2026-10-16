#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;

uniform mat4 gWorld;
uniform mat4 gView;
uniform mat4 gProjection;

out vec2 TexCoord0;

void main()
{
    gl_Position = gProjection * gView * gWorld * vec4(Position, 1.0);
    TexCoord0 = TexCoord;
}
