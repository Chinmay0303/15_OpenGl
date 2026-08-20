#version 330 core

layout (location = 0) in vec3 Position;

void main()
{
    gl_Position = vec4(1 * Position.x, 1 * Position.y, Position.z, 1.0);
}