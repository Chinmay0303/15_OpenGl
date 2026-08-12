#include <iostream>
#include <GL/freeglut.h>
using namespace std;

static void RenderSceneCB(){
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();
}

int main(int argc, char* argv[]){

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE| GLUT_RGBA | GLUT_DEPTH);


    int width = 600;
    int height = 300;

    int x = 100;
    int y = 400;

    glutInitWindowSize(width, height);
    glutInitWindowPosition(x,y);

    int window = glutCreateWindow("Tutorial 01");

    cout << "window_id = " << window << endl;

    glutDisplayFunc(RenderSceneCB);

    glutMainLoop();

    return 0;

}