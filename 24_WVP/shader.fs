#version 330 core

in vec3 VertexColour;
out vec4 FragColour;

uniform vec3 gNearColour;
uniform vec3 gFarColour;

void main()
{
    float depth = gl_FragCoord.z;

    // vec3 nearColour = vec3(0.0,0.0,1.0);
    // vec3 farColour = vec3(1.0,0.0,0.0);

    vec3 depthColour = mix(gNearColour,gFarColour,depth);

    FragColour = vec4(depthColour,1.0);
    // FragColour = vec4(VertexColour * depthColour,1.0);
}