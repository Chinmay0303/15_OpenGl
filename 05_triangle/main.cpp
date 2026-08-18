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

    // glDrawArrays(GL_POINTS,0,3);
    glDrawArrays(GL_TRIANGLES,0,3);

    glFlush();

    glDisableVertexAttribArray(0);
}

void CreateVertexBuffer(){

    Vector3f Vertices[3];

    Vertices[0] = Vector3f(-0.25f,0.0f,0.0f); // left
    Vertices[1] = Vector3f(0.0f,0.5f,0.0f); // top
    Vertices[2] = Vector3f(0.25f,0.0f,0.0f); // right

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Vertices),Vertices,GL_STATIC_DRAW);

}

int main(int argc, char ** argv){

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGBA);

    int width = 960;
    int height = 540;

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