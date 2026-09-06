#version 330 core

in vec3 ViewNormal;
in vec3 ViewPosition;

in float VertexDepth;

out vec4 FragColour;

uniform vec3 gNearColour;
uniform vec3 gFarColour;

uniform vec3 gLightPositionView;

uniform vec3 gLightAmbient;
uniform vec3 gLightDiffuse;
uniform vec3 gLightSpecular;

uniform vec3 gMaterialAmbient;
uniform vec3 gMaterialDiffuse;
uniform vec3 gMaterialSpecular;
uniform float gMaterialShininess;

uniform bool gAmbientEnabled;
uniform bool gDiffuseEnabled;
uniform bool gSpecularEnabled;

float CameraNear = 0.1;
float CameraFar = 20.0;

uniform float gCenterZ;

float DisplayNear = gCenterZ - 1.0;
float DisplayFar = gCenterZ + 1.0;

void main()
{
    float depthFactor = clamp((VertexDepth - DisplayNear) /
                             (DisplayFar - DisplayNear),
                              0.0,
                              1.0);

    vec3 depthColour = mix(gNearColour,gFarColour,depthFactor);

    vec3 N = normalize(ViewNormal);

    vec3 L = normalize(gLightPositionView - ViewPosition);

    vec3 V = normalize(-ViewPosition);

    vec3 colour = vec3(0.0);

    if (gAmbientEnabled){
        colour += gMaterialAmbient * gLightAmbient;
    }

    float diffuseFactor = max(dot(N, L), 0.0);

    if (gDiffuseEnabled){
        colour += diffuseFactor * gMaterialDiffuse * gLightDiffuse;
    }

    if (gSpecularEnabled && diffuseFactor > 0.0) {
        vec3 R = reflect(-L, N);

        float alpha = max(dot(R, V), 0.0);

        float specularFactor =pow(alpha,gMaterialShininess);

        colour += gMaterialSpecular * gLightSpecular * specularFactor;
    }

    vec3 finalColour = mix(colour,depthColour,0.3);

    FragColour = vec4(clamp(finalColour, 0.0, 1.0),1.0);
}