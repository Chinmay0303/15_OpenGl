#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;

void RenderSceneCB(){

    float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

    glDrawArrays(GL_TRIANGLES,0,6);

    glDisableVertexAttribArray(0);

    glutSwapBuffers();

    // glutPostRedisplay();
}

void CreateVertexBuffer(){

    Vector3f Vertices[6];

    Vector3f p0 = Vector3f(-0.25f,-0.25f,0.0f); // bottom left
    Vector3f p1 = Vector3f(-0.25f,0.25f,0.0f); // top left
    Vector3f p2 = Vector3f(0.25f,0.25f,0.0f); // top right
    Vector3f p3 = Vector3f(0.25f,-0.25f,0.0f); // bottom right

    Vertices[0] = p0;
    Vertices[1] = p1;
    Vertices[2] = p2;
    Vertices[3] = p0;
    Vertices[4] = p2;
    Vertices[5] = p3;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Vertices),Vertices,GL_STATIC_DRAW);
}

int main(int argc, char ** argv){

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);

    int width = 960;
    int height = 960;

    int x = 0;
    int y = 0;

    glutInitWindowPosition(x,y);
    glutInitWindowSize(width,height);

    int window_id = glutCreateWindow("Triangle");

    std::cout << "window_id : " << window_id << "\n";

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

    glutDisplayFunc(RenderSceneCB);

    glutMainLoop();

    return 0;
}