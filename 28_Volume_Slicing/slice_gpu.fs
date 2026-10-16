#version 330 core

in vec3 TexCoord0;

uniform sampler3D uVolume3D;
uniform sampler1D uColormap1D;

out vec4 FragColour;

void main()
{
    float scalar = texture(uVolume3D, TexCoord0).r;
    const float N = 256.0;
    FragColour = texture(uColormap1D, (scalar * (N - 1.0) + 0.5) / N);
}
