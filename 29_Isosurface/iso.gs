#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 15) out;

in ivec3 vCell[];

uniform sampler3D uVolume3D;
uniform isampler2D uTriTable;
uniform float uIso;
uniform vec3 uExtent;

uniform mat4 gWorld;
uniform mat4 gView;
uniform mat4 gProjection;

out float vShade;

const ivec3 CornerOffset[8] = ivec3[8](
    ivec3(0, 0, 0), // v0
    ivec3(1, 0, 0), // v1
    ivec3(1, 1, 0), // v2
    ivec3(0, 1, 0), // v3
    ivec3(0, 0, 1), // v4
    ivec3(1, 0, 1), // v5
    ivec3(1, 1, 1), // v6
    ivec3(0, 1, 1)  // v7
);

const int EdgeCorner[24] = int[24](
    0, 1, // e0
    1, 2, // e1
    2, 3, // e2
    3, 0, // e3
    4, 5, // e4
    5, 6, // e5
    6, 7, // e6
    7, 4, // e7
    0, 4, // e8
    1, 5, // e9
    2, 6, // e10
    3, 7  // e11
);

vec3 ComputeEdgeVertex(int e, float v[8], ivec3 cell, vec3 texDimsMinus1)
{
    int a = EdgeCorner[2 * e];
    int b = EdgeCorner[2 * e + 1];
    float t = (uIso - v[a]) / (v[b] - v[a]);
    vec3 pGrid = mix(vec3(cell + CornerOffset[a]), vec3(cell + CornerOffset[b]), t);
    return (pGrid / texDimsMinus1) * uExtent;
}

void main()
{
    ivec3 cell = vCell[0];
    vec3 texDimsMinus1 = vec3(textureSize(uVolume3D, 0) - ivec3(1));

    float v[8];
    int cubeindex = 0;
    for (int c = 0; c < 8; ++c) {
        v[c] = texelFetch(uVolume3D, cell + CornerOffset[c], 0).r;
        if (v[c] < uIso) {
            cubeindex |= (1 << c);
        }
    }

    if (cubeindex == 0 || cubeindex == 255) {
        return;
    }

    for (int t = 0; t < 15; t += 3) {
        int e0 = texelFetch(uTriTable, ivec2(t, cubeindex), 0).r;
        if (e0 < 0) break;
        int e1 = texelFetch(uTriTable, ivec2(t + 1, cubeindex), 0).r;
        int e2 = texelFetch(uTriTable, ivec2(t + 2, cubeindex), 0).r;

        vec3 p0 = ComputeEdgeVertex(e0, v, cell, texDimsMinus1);
        vec3 p1 = ComputeEdgeVertex(e1, v, cell, texDimsMinus1);
        vec3 p2 = ComputeEdgeVertex(e2, v, cell, texDimsMinus1);

        vec3 v0 = (gView * gWorld * vec4(p0, 1.0)).xyz;
        vec3 v1 = (gView * gWorld * vec4(p1, 1.0)).xyz;
        vec3 v2 = (gView * gWorld * vec4(p2, 1.0)).xyz;

        vec3 N = cross(v1 - v0, v2 - v0);
        float lenN = length(N);
        if (lenN > 0.000001) N /= lenN;
        else N = vec3(0.0, 0.0, 1.0);

        vec3 triCentroid = (v0 + v1 + v2) * (1.0 / 3.0);
        vec3 L = -triCentroid;
        float lenL = length(L);
        if (lenL > 0.000001) L /= lenL;
        else L = vec3(0.0, 0.0, 1.0);

        float shade = abs(dot(N, L));

        gl_Position = gProjection * vec4(v0, 1.0);
        vShade = shade;
        EmitVertex();

        gl_Position = gProjection * vec4(v1, 1.0);
        vShade = shade;
        EmitVertex();

        gl_Position = gProjection * vec4(v2, 1.0);
        vShade = shade;
        EmitVertex();

        EndPrimitive();
    }
}

