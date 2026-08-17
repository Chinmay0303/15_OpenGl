#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;

static void RenderScene(){

    static GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    glClearColor(red, green, blue, alpha);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    // attribute index, # of elements, type, normalise to 0 to 1, stride, offset pointer

    glDrawArrays(GL_POINTS,0,2);
    // , index of first vertex, # of vertices to process

    glDisableVertexAttribArray(0);

    glFlush();
}

static void CreateVertexBuffer(){
    Vector3f Vertices[2];
    Vertices[0] = Vector3f(0.0f,0.0f,0.0f);
    Vertices[1] = Vector3f(0.2f,0.0f,0.0f);

    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices),Vertices,GL_STATIC_DRAW);
}


int main(int argc, char** argv){

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