#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint gWorldLocation;
int N = 720;

void CreateVertexBuffer(){

    std::vector<Vector3f> Vertices(N);
    Vector3f p0 = Vector3f(0.5f,0.0f,0.0f); //right

    int i = 0;
    float Delta = 360.0f/N;
    float AngleInDegrees = 0.0f;
    
    Matrix3f RotateDot;

    while(i < N){

        float AngleInRadians = 2.0f * M_PI * AngleInDegrees/360.0f;

        // float newX = cosf(AngleInRadians)*p0.x - sinf(AngleInRadians)*p0.y;
        // float newY = sinf(AngleInRadians)*p0.x + cosf(AngleInRadians)*p0.y;
        // float newZ = p0.z;

        // Vertices[i] = Vector3f(newX,newY,newZ);

        // to rotate around z axis
        RotateDot.m[0][0] = cosf(AngleInRadians); RotateDot.m[0][1] = -sinf(AngleInRadians); RotateDot.m[0][2] = 0.0f;
        RotateDot.m[1][0] = sinf(AngleInRadians); RotateDot.m[1][1] = cosf(AngleInRadians); RotateDot.m[1][2] = 0.0f; 
        RotateDot.m[2][0] = 0.0f; RotateDot.m[2][1] = 0.0f; RotateDot.m[2][2] = 1.0f;

        // Rotate.m[0][0] = cosf(AngleInRadians); Rotate.m[0][1] = -sin(AngleInRadians); Rotate.m[0][2] = 0.0f; Rotate.m[0][3] = 0.0f; 
        // Rotate.m[1][0] = sinf(AngleInRadians); Rotate.m[1][1] = cosf(AngleInRadians); Rotate.m[1][2] = 0.0f; Rotate.m[1][3] = 0.0f; 
        // Rotate.m[2][0] = 0.0f; Rotate.m[2][1] = 0.0f; Rotate.m[2][2] = 1.0f; Rotate.m[2][3] = 0.0f; 
        // Rotate.m[3][0] = 0.0f; Rotate.m[3][1] = 0.0f; Rotate.m[3][2] = 0.0f; Rotate.m[3][3] = 1.0f; 

        Vertices[i] = RotateDot * p0;

        i++;
        AngleInDegrees += Delta;
    }


    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,Vertices.size() * sizeof(Vector3f),Vertices.data(),GL_STATIC_DRAW);
}

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT);

    Matrix4f World;
    World.m[0][0] = 1.0f; World.m[0][1] = 0.0f; World.m[0][2] = 0.0f; World.m[0][3] = 0.0f; 
    World.m[1][0] = 0.0f; World.m[1][1] = 1.0f; World.m[1][2] = 0.0f; World.m[1][3] = 0.0f; 
    World.m[2][0] = 0.0f; World.m[2][1] = 0.0f; World.m[2][2] = 1.0f; World.m[2][3] = 0.0f; 
    World.m[3][0] = 0.0f; World.m[3][1] = 0.0f; World.m[3][2] = 0.0f; World.m[3][3] = 1.0f; 

    glUniformMatrix4fv(gWorldLocation,1,GL_TRUE,&World.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

    static int j = 0;
    int step = 1;

    int start = std::max(0,j-N);
    int end = std::min(N,j);
    int count = end - start;

    if(count > 0)
    {
        glDrawArrays(GL_POINTS,start,count);
    }
    else{
        std::cout << "count: " << count << ", start: " << start << ", end: " << end << "\n";
    }

    j = j + step;

    if(j > 2*N){
        j = 0;
    }

    glDisableVertexAttribArray(0);
    glutSwapBuffers();
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

    gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
    if (gWorldLocation == -1) {
        printf("Error getting uniform location of 'gWorld'\n");
        exit(1);
    }
    else{
        std::cout << "gWorldLocation: " << gWorldLocation << "\n";
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

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    int width = 960;
    int height = 960;

    int x = 0;
    int y = 0;

    glutInitWindowSize(width,height);
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

    CreateVertexBuffer();

    CompileShaders();

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    glClearColor(red,green,blue,alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glutDisplayFunc(RenderSceneCB);

    glutMainLoop();

    return 0;
}