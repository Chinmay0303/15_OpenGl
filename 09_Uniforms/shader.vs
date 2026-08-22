#version 330 core

// attribute variable that will receive data from a vertex buffer object
layout(location = 0) in vec3 position;

// uniform variable that will be set from our application
uniform float gScale;

// Attribute variables contain data that is vertex specific so they are reloaded with a new value from the vertex buffer for each shader invocation.
// Uniform variables remain constant across the entire draw call.
// Uniform variables: lighting parameters, transformation matrices, etc.

void main(){
    // gl_Position = vec4(gScale*position.x, gScale*position.y, gScale*position.z, 1.0);
    // gl_Position = vec4(gScale + gScale*position.x, gScale*position.y, gScale*position.z, 1.0);
    mat4 M = mat4(
            gScale,0,0,0, // first column
            0,gScale,0,0, // second column
            0,0,1,0,      // third column
            gScale,gScale,0,1 // fourth column
    );

    gl_Position = M * vec4(position,1.0);
}