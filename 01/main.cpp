#include <iostream>
#include <GL/freeglut.h>
// using namespace std;
using std::cout;

static void RenderSceneCB(){

    static int scene = 0;
    // initialized only once, and is not destroyed when the function returns a value, it retains its value between function calls
    // a function can also return the pointer to a static local variable, unlike normal variables which are destroyed when the function exits

    static GLclampf red = 1.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
    // The color has four channels (RGBA) and it is specified as a normalized value between 0.0 and 1.0
    glClearColor(red, green, blue, alpha); // colour buffer now stores this; default is all 0.0f

    glClear(GL_COLOR_BUFFER_BIT);
    // clear the frame buffer using the colour specified above

    // glFlush(); // for only single buffer

    if(scene % 100 == 0){
        cout << "Render_Scene_" << scene << "\n";
    }
    scene += 1;

    glutPostRedisplay(); // allows the RenderSceneCB function to be called continuously by glut

    glutSwapBuffers(); // swaps the roles of front and back buffers, allowing for double buffering to prevent flickering
}

int main(int argc, char* argv[]){

    glutInit(&argc,argv);

    // bitmasking ??
    // glutInitDisplayMode(GLUT_DEPTH);
    // glutInitDisplayMode(GLUT_DOUBLE| GLUT_RGBA | GLUT_DEPTH);
    // glutInitDisplayMode(GLUT_SINGLE| GLUT_RGBA);
    glutInitDisplayMode(GLUT_DOUBLE| GLUT_RGBA);
    // single frame buffer and double frame buffers(front and back)
    // double buffer allows us to draw to a back buffer while the front buffer is being displayed
    // colour buffer
    // depth buffer

    int width = 600;
    int height = 300;

    int x = 100;
    int y = 400;

    glutInitWindowSize(width, height);
    glutInitWindowPosition(x,y);

    int window = glutCreateWindow("Tutorial 01"); // returns an integer value and takes a string input

    cout << "window_id = " << window << "\n";

    glutDisplayFunc(RenderSceneCB); // event call back function, called continuously by glut internal loop

    glutMainLoop(); // begin glut internal loop

    return 0;

}