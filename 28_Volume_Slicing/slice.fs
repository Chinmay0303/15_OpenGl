#version 330 core

in vec2 TexCoord0;
uniform sampler2D gSliceImage;
out vec4 FragColour;

void main()
{
    FragColour = texture(gSliceImage, TexCoord0);
}
