#version 330 core

in float vShade;

uniform vec3 uColour;
uniform float uAmbient;
uniform float uDiffuse;

out vec4 FragColour;

void main()
{
    vec3 color = uColour * (uAmbient + uDiffuse * vShade);
    FragColour = vec4(color, 1.0);
}

