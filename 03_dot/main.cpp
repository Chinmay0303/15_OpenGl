#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

static void RenderScene(){

    static GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    glClearColor(red, green, blue, alpha);

    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
}

static void CreateVertexBuffer(){
    Vector3f Vertices[1];
    Vertices[0] = Vector3f();
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