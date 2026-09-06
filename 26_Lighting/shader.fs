#version 330 core

in vec3 ViewNormal;
in vec3 ViewPosition;

out vec4 FragColour;

uniform vec3 gNearColour;
uniform vec3 gFarColour;

uniform vec3 gLightPositionView;

uniform vec3 gLightAmbient;
uniform vec3 gLightDiffuse;

uniform vec3 gMaterialAmbient;
uniform vec3 gMaterialDiffuse;

uniform bool gAmbientEnabled;
uniform bool gDiffuseEnabled;

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

    vec3 N = normalize(ViewNormal);

    vec3 L = normalize(gLightPositionView - ViewPosition);

    vec3 colour = vec3(0.0);

    if (gAmbientEnabled){
        colour += gMaterialAmbient * gLightAmbient;
    }

    if (gDiffuseEnabled){
        float diffuseFactor = max(dot(N, L), 0.0);

        colour += diffuseFactor * gMaterialDiffuse * gLightDiffuse;
    }

    vec3 finalColour = mix(colour,depthColour,0.5);

    FragColour = vec4(clamp(finalColour, 0.0, 1.0),1.0);
}