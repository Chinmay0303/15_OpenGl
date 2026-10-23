#version 330 core

uniform vec3 gLineColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(gLineColor, 1.0);
}

