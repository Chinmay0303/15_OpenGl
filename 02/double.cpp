#include <iostream>
#include <GL/freeglut.h>
using std::cout;

void RenderScene(){

    static GLclampf red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 1.0f;

    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

    static GLclampf step = 0.001f;

    // red += step;
    green += step;
    blue += step;

    if(green <= 0.0f || green >= 1.0f){
        step = -step;
    }

    glutPostRedisplay();
}

int main(int argc, char** argv){

    cout << "Hello Window!" << "\n";

    glutInit(&argc, argv);

    // glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    int x = 600;
    int y = 200;

    int width = 480;
    int height = 360;

    glutInitWindowSize(width, height);
    glutInitWindowPosition(x,y);

    int window = glutCreateWindow("Double Buffer");

    cout << "window_id = "<< window << "\n";

    glutDisplayFunc(RenderScene);

    glutMainLoop();

    return 0;
}