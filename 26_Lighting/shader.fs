#version 330 core

in vec3 VertexNormal;
out vec4 FragColour;

uniform vec3 gNearColour;
uniform vec3 gFarColour;

float CameraNear = 0.1;
float CameraFar = 20.0;

uniform float gCenterZ;

float DisplayNear = gCenterZ - 1.0;
float DisplayFar = gCenterZ + 1.0;

void main()
{
    float ndcDepth = gl_FragCoord.z * 2.0 - 1.0;

    float eyeDistance =
        (2.0 * CameraNear * CameraFar) /
        (CameraFar + CameraNear -
         ndcDepth * (CameraFar - CameraNear));

    float depthFactor = clamp(
        (eyeDistance - DisplayNear) /
        (DisplayFar - DisplayNear),
        0.0,
        1.0
    );

    // depthFactor = smoothstep(0.0, 1.0, depthFactor);

    vec3 depthColour = mix(gNearColour, gFarColour, depthFactor);

    vec3 normal = normalize(VertexNormal);

    // Convert [-1, 1] to [0, 1].
    vec3 normalColour = normal * 0.5 + 0.5;

    vec3 finalColour = mix(normalColour,depthColour,0.5);

    FragColour = vec4(clamp(finalColour, 0.0, 1.0),1.0);
}