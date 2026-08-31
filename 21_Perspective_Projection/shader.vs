#version 330 core

layout (location=0) in vec3 Position;
layout (location=1) in vec3 inColour;

out vec4 Colour;

uniform mat4 gWorld;

void main(){
    gl_Position = gWorld*vec4(Position,1.0f);
    Colour = vec4(inColour,1.0f);
}