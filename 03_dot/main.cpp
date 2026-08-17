#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;

// Vertex Buffer Object: used to store vertices
// VBO stores the handle of the vertex buffer object (32 bits unsigned int)
// integer id not the actual data of the buffer

static void RenderScene(){

    static GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    glClearColor(red, green, blue, alpha);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    // good practice to bind the buffer before using it, even if it is already bound
    
    glEnableVertexAttribArray(0);
    // enable the vertex attribute array at index 0
    // index 0 is the position of the vertex 
    // vertex attributes: per-vertex inputs to the shader pipeline

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    // this calls tells the pipeline how to interpret the data inside the buffer.
    // Parameters: attribute index, # of components (3 for x,y,z), datatype, normalise to 0 to 1, stride, offset pointer
    // Stride: number of bytes between two instances of that attribute in the buffer.
    // Offset pointer: offset where the data begins in the buffer. In this case, it is 0 because the data starts at the beginning of the buffer.

    glDrawArrays(GL_POINTS,0,2);
    // call to draw the vertices in the buffer (OrderedDraw)
    // Parameters: 
    // primitive type: every vertex is a point
    // index of the first vertex to draw
    // number of vertices to draw

    glDisableVertexAttribArray(0);
    // good practice to disable the vertex attribute array when it is not immediately needed anymore

    glFlush();
}

static void CreateVertexBuffer(){
    Vector3f Vertices[2];
    Vertices[0] = Vector3f(0.0f,0.0f,0.0f);
    Vertices[1] = Vector3f(0.2f,0.0f,0.0f);

    glGenBuffers(1,&VBO);
    // openGL defines several glGen* functions to generate objects of various types.
    // Two parameters: # of objects to generate, address of an array of GLuints to store the handles that the driver allocates.
    // At this point, the buffer is generic

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    // Bind the handle to a target name and then execute commands on that target.
    // These commands will affect the bounded handle until another one is bound in its stead or the call above takes zero as the handle, which unbinds the current handle from the target.
    // The target GL_ARRAY_BUFFER means that the buffer will contain an array of vertices.

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices),Vertices,GL_STATIC_DRAW);
    // after binding our object we fill it with data.
    // Parameters: target name, size of data in bytes, address of the array of vertices, a flag that indicates the usage pattern
    // (GL_STATIC_DRAW means that the data will most likely not change at all or very rarely)
}


int main(int argc, char** argv){

    std::cout << "sizeof(VBO): " << sizeof(VBO) << "\n";

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_SINGLE| GLUT_RGBA);

    int width = 480;
    int height = 360;

    int x = 200;
    int y = 200;

    glutInitWindowSize(width, height);
    glutInitWindowPosition(x,y);

    int window_id = glutCreateWindow("Tutorial Dot");

    std::cout << "Window_id = " << window_id << "\n";

    // Must be done after glut is initialized

    // check if glewInit() is successful
    GLenum res = glewInit();
    if(res != GLEW_OK){
        std::cout << "Error: " << glewGetErrorString(res) << "\n";
        return 1;
    }
    else
        std::cout << "glewInit " << res << "\n";

    CreateVertexBuffer();

    glutDisplayFunc(RenderScene);

    glutMainLoop();

    return 0;
}