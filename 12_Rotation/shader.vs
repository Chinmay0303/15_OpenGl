#version 330 core

layout (location = 0) in vec3 Position;
uniform mat4 gRotate;

void main(){
    gl_Position = gRotate * vec4(Position,1.0);
}