#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint gWorldLocation;

float scaleX = 0.0f;
float scaleY = 0.0f;
float deltaScaleX = 0.004f;
float deltaScaleY = 0.002f;

void CreateVertexBuffer(){

    std::vector<Vector3f> Vertices(3);
    Vector3f p0 = Vector3f(-0.5f,0.0f,0.0f); //left
    Vector3f p1 = Vector3f(0.5f,0.0f,0.0f); //right
    Vector3f p2 = Vector3f(0.0f,0.5f,0.0f); //top

    Vertices[0] = p0;
    Vertices[1] = p1;
    Vertices[2] = p2;

    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,Vertices.size() * sizeof(Vector3f),Vertices.data(),GL_STATIC_DRAW);
}

static void UpdateScale(){
    scaleX += deltaScaleX;
    scaleY += deltaScaleY;

    if(scaleX >= 2.0f || scaleX <= -2.0f){
        deltaScaleX = -1.0f * deltaScaleX;
    }
    if(scaleY >= 2.0f || scaleY <= -2.0f){
        deltaScaleY = -1.0f * deltaScaleY;
    }
}

static Matrix4f ScalingExample()
{
    Matrix4f Scaling(scaleX, 0.0f,  0.0f,  0.0f,
                     0.0f,  scaleY, 0.0f,  0.0f,
                     0.0f,  0.0f,  1.0f, 0.0f,
                     0.0f,  0.0f,  0.0f,  1.0f);

    return Scaling;
}

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT);

    UpdateScale();
    Matrix4f S = ScalingExample();

    Matrix4f World = S;

    // Matrix4f World(1.0f,0.0f,0.0f,0.0f,
    //                0.0f,1.0f,0.0f,0.0f,
    //                0.0f,0.0f,1.0f,0.0f,
    //                0.0f,0.0f,0.0f,1.0f
    
    // );

    glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

    glDrawArrays(GL_TRIANGLES,0,3);

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