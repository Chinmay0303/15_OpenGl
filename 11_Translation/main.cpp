#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint gTranslateLocation;

void CreateVertexBuffer(){

    Vector3f Vertices[8];

    Vector3f p0 = Vector3f(-0.5f,0.5f,0.0f); //top-left
    Vector3f p1 = Vector3f(0.5f,0.5f,0.0f); //top-right
    Vector3f p2 = Vector3f(0.5f,-0.5f,0.0f); //bottom-right
    Vector3f p3 = Vector3f(-0.5f,-0.5f,0.0f); //bottom-left

    Vertices[0] = p0;
    Vertices[1] = p1;
    Vertices[2] = p1;
    Vertices[3] = p2;
    Vertices[4] = p2;
    Vertices[5] = p3;
    Vertices[6] = p3;
    Vertices[7] = p0;

    glCreateBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(Vertices),Vertices,GL_STATIC_DRAW);
}

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT);

    static float translateX = 0.0f;
    static float translateY = 0.0f;

    static float stepX = 0.0025f;
    static float stepY = 0.005f;

    Matrix4f Translate;

    Translate.m[0][0] = 0.5f; Translate.m[0][1] = 0.0f; Translate.m[0][2] = 0.0f; Translate.m[0][3] = translateX; 
    Translate.m[1][0] = 0.0f; Translate.m[1][1] = 0.5f; Translate.m[1][2] = 0.0f; Translate.m[1][3] = translateY; 
    Translate.m[2][0] = 0.0f; Translate.m[2][1] = 0.0f; Translate.m[2][2] = 1.0f; Translate.m[2][3] = 0.0f; 
    Translate.m[3][0] = 0.0f; Translate.m[3][1] = 0.0f; Translate.m[3][2] = 0.0f; Translate.m[3][3] = 1.0f; 

    glUniformMatrix4fv(gTranslateLocation,1,GL_TRUE,&Translate.m[0][0]);

    translateX += stepX;
    translateY += stepY;

    if((translateX >= 1.0) || (translateX <= -1.0)){
        stepX = -1.0*stepX;
    }

    if((translateY >= 1.0) || (translateY <= -1.0)){
        stepY = -1.0*stepY;
    }

    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    glDrawArrays(GL_LINES,0,8);

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

    gTranslateLocation = glGetUniformLocation(ShaderProgram, "gTranslate");
    if (gTranslateLocation == -1) {
        printf("Error getting uniform location of 'gScale'\n");
        exit(1);
    }
    else{
        std::cout << "gTranslateLocation: " << gTranslateLocation << "\n";
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