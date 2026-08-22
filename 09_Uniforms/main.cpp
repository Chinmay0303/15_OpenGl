#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLint gScaleLocation;

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT);

    static float Scale = 0.0f;
    static float Delta = 0.005f;

    Scale += Delta;

    if((Scale >= 0.5f) || (Scale <= -0.5f)){
        Delta *= -1.0f;
    }

    glUniform1f(gScaleLocation,Scale);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    glDrawArrays(GL_TRIANGLES,0,3);

    glDisableVertexAttribArray(0);
    glutSwapBuffers();
    glutPostRedisplay();
}

void CreateVertexBuffer(){

    Vector3f Vertices[3];

    Vertices[0] = Vector3f(-0.5f,-0.5f,0.0f); //bottom-left
    Vertices[1] = Vector3f(0.0f,0.5f,0.0f); //top
    Vertices[2] = Vector3f(0.5f,-0.5f,0.0f); //bottom-right

    glCreateBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Vertices),Vertices,GL_STATIC_DRAW);
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

    gScaleLocation = glGetUniformLocation(ShaderProgram, "gScale");
    if (gScaleLocation == -1) {
        printf("Error getting uniform location of 'gScale'\n");
        exit(1);
    }
    else{
        std::cout << "gScaleLocation: " << gScaleLocation << "\n";
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
    int height = 540;

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