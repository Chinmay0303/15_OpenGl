#version 330 core

uniform sampler3D uVolume3D;

out ivec3 vCell;

void main()
{
    ivec3 d = textureSize(uVolume3D, 0) - ivec3(1);
    int vid = gl_VertexID;
    int xy = d.x * d.y;

    int cz = vid / xy;
    int rem = vid % xy;
    int cy = rem / d.x;
    int cx = rem % d.x;

    vCell = ivec3(cx, cy, cz);
    gl_Position = vec4(0.0);
}

