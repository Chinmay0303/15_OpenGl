#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"
#include "world.h"
#include "view.h"
#include "project.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

GLuint VBO;
GLuint IBO;
GLuint gWVPLocation;

World CubeWorld;
View CubeView;

GLenum Mode = GL_FILL;
bool CullState = true;

float FOV = 45.0f;
float zNear = 1.0f;
float zFar = 10.0f;

Project CubeProject(FOV,WINDOW_WIDTH,WINDOW_HEIGHT,zNear,zFar);

struct AnimationState {
    bool x = false;
    bool y = false;
    bool z = false;
    bool arbitrary = true;
    bool paused = false;
};

static AnimationState animationState;

static Vector3f CreateRandomAxis()
{
    // (rand()/RAND_MAX) generates a float between 0 and 1
    Vector3f axis(
        ((float)rand()/RAND_MAX) * 2.0f - 1.0f,
        ((float)rand()/RAND_MAX) * 2.0f - 1.0f,
        ((float)rand()/RAND_MAX) * 2.0f - 1.0f);

    return axis;
}

Vector3f randomAxis;

static void UpdateAnimation( Vector3f arbitraryAxis)
{
    if (animationState.paused) {
        return;
    }

    float angleStep = 0.5f;

    if (animationState.x) {
        CubeWorld.Rotate(angleStep, 0.0f, 0.0f);
    }

    if (animationState.y) {
        CubeWorld.Rotate(0.0f, angleStep, 0.0f);
    }

    if (animationState.z) {
        CubeWorld.Rotate(0.0f, 0.0f, angleStep);
    }

    if (animationState.arbitrary) {
        CubeWorld.RotateAroundArbitraryAxis(arbitraryAxis, angleStep);
    }
}

struct Vertex {
    Vector3f pos;
    Vector3f color;

    Vertex() {}

    Vertex(float x, float y, float z)
    {
        pos = Vector3f(x, y, z);

        float red   = RandomFloat();
        float green = RandomFloat();
        float blue  = RandomFloat();
        color = Vector3f(red, green, blue);
    }
};

static void CreateVertexBuffer()
{
    Vertex Vertices[8];

    Vertices[0] = Vertex(0.5f, 0.5f, 0.5f);
    Vertices[1] = Vertex(-0.5f, 0.5f, -0.5f);
    Vertices[2] = Vertex(-0.5f, 0.5f, 0.5f);
    Vertices[3] = Vertex(0.5f, -0.5f, -0.5f);
    Vertices[4] = Vertex(-0.5f, -0.5f, -0.5f);
    Vertices[5] = Vertex(0.5f, 0.5f, -0.5f);
    Vertices[6] = Vertex(0.5f, -0.5f, 0.5f);
    Vertices[7] = Vertex(-0.5f, -0.5f, 0.5f);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}

static void CreateIndexBuffer()
{
    unsigned int Indices[] = {
                              0, 1, 2,
                              1, 3, 4,
                              5, 6, 3,
                              7, 3, 6,
                              2, 4, 7,
                              0, 7, 6,
                              0, 5, 1,
                              1, 5, 3,
                              5, 0, 6,
                              7, 4, 3,
                              2, 1, 4,
                              0, 2, 7
    };

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
}

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    UpdateAnimation(randomAxis);
    
    Matrix4f W = CubeWorld.GetMatrix();

    Matrix4f V = CubeView.GetMatrix();

    Matrix4f P = CubeProject.GetMatrix();

    Matrix4f WVP = P * V * W;

    glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &WVP.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6 * sizeof(float),0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3 * sizeof(float)));

    // glPolygonMode(GL_FRONT,GL_FILL);
    glPolygonMode(GL_FRONT_AND_BACK,Mode);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glutSwapBuffers();
    glutPostRedisplay();
}

static void KeyboardCB(unsigned char key, int mouse_x, int mouse_y)
{
    float rotationStep = 5.0f;
    float scaleFactor = 1.1f;

    switch (key) {

        case '1':
            CubeWorld.Rotate(rotationStep, 0.0f, 0.0f);
            break;

        case '2':
            CubeWorld.Rotate(-rotationStep, 0.0f, 0.0f);
            break;

        case '3':
            CubeWorld.Rotate(0.0f, rotationStep, 0.0f);
            break;

        case '4':
            CubeWorld.Rotate(0.0f, -rotationStep, 0.0f);
            break;

        case '5':
            CubeWorld.Rotate(0.0f, 0.0f, rotationStep);
            break;

        case '6':
            CubeWorld.Rotate(0.0f, 0.0f, -rotationStep);
            break;

        case '-':
        case '_':
            {
                CubeWorld.ScaleUniform(1.0f/scaleFactor);
            }
            break;

        case '+':
        case '=':
            {
                CubeWorld.ScaleUniform(scaleFactor);
            }
            break;

        // Reset model transform.
        case '0':
            {
                CubeWorld.ResetTransform();
                animationState.x = false;
                animationState.y = false;
                animationState.z = false;
                animationState.arbitrary = false;
                animationState.paused = false;
            }
            break;

        case 'x':
        case 'X':
            animationState.x = !animationState.x;
            break;

        case 'y':
        case 'Y':
            animationState.y = !animationState.y;
            break;

        case 'z':
        case 'Z':
            animationState.z = !animationState.z;
            break;

        case 'r':
        case 'R':
            {
                if(!animationState.arbitrary)
                    randomAxis = CreateRandomAxis();
                animationState.arbitrary = !animationState.arbitrary;
            }
            break;

        case ' ':
            animationState.paused = !animationState.paused;
            break;

        case 'f':
        case 'F':
            {
                    Mode = GL_FILL;
            }
            break;

        case 'w':
        case 'W':
            {
                    Mode = GL_LINE;
            }
            break;

        case 'c':
        case 'C':
            {
                   if(CullState){
                        glDisable(GL_CULL_FACE);
                        CullState = false;
                   }
                   else{
                        glEnable(GL_CULL_FACE);
                        CullState = true;
                   }

            }
            break;

        default:
        {
            CubeView.OnKeyboard(key);
        }
    }
    glutPostRedisplay();
}


static void SpecialKeyboardCB(int key, int mouse_x, int mouse_y)
{
    float movement = 0.1f;

    switch (key) {
        case GLUT_KEY_LEFT:
            CubeWorld.Translate(-movement, 0.0f, 0.0f);
            break;

        case GLUT_KEY_RIGHT:
            CubeWorld.Translate(movement, 0.0f, 0.0f);
            break;

        case GLUT_KEY_UP:
            CubeWorld.Translate(0.0f, movement, 0.0f);
            break;

        case GLUT_KEY_DOWN:
            CubeWorld.Translate(0.0f, -movement, 0.0f);
            break;
    }

    glutPostRedisplay();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }

    const GLchar* p[1];
    p[0] = pShaderText;

    GLint Lengths[1];
    Lengths[0] = (GLint)strlen(pShaderText);

    glShaderSource(ShaderObj, 1, p, Lengths);

    glCompileShader(ShaderObj);

    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }

    glAttachShader(ShaderProgram, ShaderObj);
}

const char* pVSFileName = "shader.vs";
const char* pFSFileName = "shader.fs";

static void CompileShaders()
{
    GLuint ShaderProgram = glCreateProgram();

    if (ShaderProgram == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    std::string vs, fs;

    if (!ReadFile(pVSFileName, vs)) {
        exit(1);
    };

    AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);

    if (!ReadFile(pFSFileName, fs)) {
        exit(1);
    };

    AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(ShaderProgram);

    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    gWVPLocation = glGetUniformLocation(ShaderProgram, "gWVP");
    if (gWVPLocation == -1) {
        printf("Error getting uniform location of 'gWVP'\n");
        exit(1);
    }
    else{
        std::cout << "gWVPLocation: " << gWVPLocation << "\n";
    }

    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glUseProgram(ShaderProgram);
}

int main(int argc, char** argv){

    srand(time(0));

    randomAxis = CreateRandomAxis();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    // int width = 960;
    // int height = 960;

    int x = 0;
    int y = 0;
 
    CubeWorld.SetPosition(0.0f,0.0f,-2.0f);
    CubeWorld.SetScale(1.0f,1.0f,1.0f);

    CubeView.SetPosition(0.0f, 0.0f, 0.0f);
    CubeView.SetForward(0.0f, 0.0f, -1.0f);
    CubeView.SetUp(0.0f, 1.0f, 0.0f);

    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutInitWindowPosition(x,y);

    int window_id = glutCreateWindow("Uniforms");

    std::cout << "window_id : " << window_id << "\n";

    GLenum res = glewInit();
    
    if(res != GLEW_OK){
        std::cout << "Error: " << glewGetErrorString(res) << "\n";
        return 1;
    }
    else
    std::cout << "glewInit " << res << "\n";
    
    glEnable(GL_DEPTH_TEST);

    // glDisable(GL_CULL_FACE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    CreateVertexBuffer();
    CreateIndexBuffer();

    CompileShaders();

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    glClearColor(red,green,blue,alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glutDisplayFunc(RenderSceneCB);

    glutKeyboardFunc(KeyboardCB);
    glutSpecialFunc(SpecialKeyboardCB);

    glutMainLoop();

    return 0;
}