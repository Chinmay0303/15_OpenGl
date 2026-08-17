#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
int N = 1000;

void RenderScene(){
    static float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

    glDrawArrays(GL_POINTS,0,N);

    glDisableVertexAttribArray(0);

    // glFlush();
    glutSwapBuffers();
    glutPostRedisplay();
}

void CreateVertexBuffer(){
    Vector3f Vertices[N];

    int i = 0;
    while(i < N){

        static float x = 0.0f, y = 0.0f, z = 0.0f;

        x = i * 0.001f;

        Vertices[i] = Vector3f(x,y,z);

        i++;
    }

    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Vertices),Vertices,GL_STATIC_DRAW);

}

int main(int argc, char**argv){

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);

    int width = 960;
    int height = 540;

    int x = 0;
    int y = 0;

    glutInitWindowPosition(x,y);
    glutInitWindowSize(width,height);

    int window_id = glutCreateWindow("Dot Tutorial"); 

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

    glutDisplayFunc(RenderScene);

    glutMainLoop();


    return 0;
}

