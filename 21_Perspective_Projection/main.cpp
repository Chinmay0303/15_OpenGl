#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint IBO;
GLuint gWorldLocation;

float scale = 1.0f;
float deltaScale = 0.0025f;

float location = 1.5f;
float deltaLoc = 0.005f;

float AngleZInDegrees = 0.0f;
float DeltaZInDegrees = 0.5f;
float AngleXInDegrees = 0.0f;
float DeltaXInDegrees = -0.5f;
float AngleYInDegrees = 0.0f;
float DeltaYInDegrees = 0.5f;

float FovInDegrees = 90.0f;

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

static void UpdateScale(){

    if(scale >= 2.0f || scale <= 0.0f){
        deltaScale *= -1.0f; 
    }

    scale += deltaScale;
}
static Matrix4f ScaleMatrix(){
    Matrix4f S;
    S.m[0][0] = scale; S.m[0][1] = 0.0f; S.m[0][2] = 0.0f;S.m[0][3] = 0.0f;
    S.m[1][0] = 0.0f; S.m[1][1] = scale; S.m[1][2] = 0.0f;S.m[1][3] = 0.0f;
    S.m[2][0] = 0.0f; S.m[2][1] = 0.0f; S.m[2][2] = scale;S.m[2][3] = 0.0f;
    S.m[3][0] = 0.0f; S.m[3][1] = 0.0f; S.m[3][2] = 0.0f;S.m[3][3] = 1.0f;

    return S;
}

static void UpdateLocation(){

    if(location >= 3.0f || location <= 0.5f){
        deltaLoc *= -1.0f; 
    }

    location += deltaLoc;
}
static Matrix4f TranslateMatrix(){
    Matrix4f T;
    T.m[0][0] = 1.0f; T.m[0][1] = 0.0f; T.m[0][2] = 0.0f;T.m[0][3] = 0.0f;
    T.m[1][0] = 0.0f; T.m[1][1] = 1.0f; T.m[1][2] = 0.0f;T.m[1][3] = 0.0f;
    T.m[2][0] = 0.0f; T.m[2][1] = 0.0f; T.m[2][2] = 1.0f;T.m[2][3] = location;
    T.m[3][0] = 0.0f; T.m[3][1] = 0.0f; T.m[3][2] = 0.0f;T.m[3][3] = 1.0f;

    return T;
}

static void UpdateAngleZ(){
    AngleZInDegrees += DeltaZInDegrees;
    
    if(AngleZInDegrees >= 360.0f){
        AngleZInDegrees = 0.0f + DeltaZInDegrees;
    }
    if(AngleZInDegrees <= 0.0f){
        AngleZInDegrees = 360.0f + DeltaZInDegrees;
    }
}
static Matrix4f RotateZMatrix(){

    float AngleZInRadians = (2.0f * M_PI / 360.0f) * AngleZInDegrees;

    Matrix4f Rz;
    Rz.m[0][0] = cosf(AngleZInRadians); Rz.m[0][1] = -sinf(AngleZInRadians); Rz.m[0][2] = 0.0f;Rz.m[0][3] = 0.0f;
    Rz.m[1][0] = sinf(AngleZInRadians); Rz.m[1][1] = cosf(AngleZInRadians); Rz.m[1][2] = 0.0f;Rz.m[1][3] = 0.0f;
    Rz.m[2][0] = 0.0f; Rz.m[2][1] = 0.0f; Rz.m[2][2] = 1.0f;Rz.m[2][3] = 0.0f;
    Rz.m[3][0] = 0.0f; Rz.m[3][1] = 0.0f; Rz.m[3][2] = 0.0f;Rz.m[3][3] = 1.0f;

    return Rz;
}

static void UpdateAngleX(){
    AngleXInDegrees += DeltaXInDegrees;
    
    if(AngleXInDegrees >= 360.0f){
        AngleXInDegrees = 0.0f + DeltaXInDegrees;
    }
    if(AngleXInDegrees <= 0.0f){
        AngleXInDegrees = 360.0f + DeltaXInDegrees;
    }
}
static Matrix4f RotateXMatrix(){

    float AngleXInRadians = (2.0f * M_PI / 360.0f) * AngleXInDegrees;

    Matrix4f Rx;
    Rx.m[0][0] = 1.0f; Rx.m[0][1] = 0.0f; Rx.m[0][2] = 0.0f;Rx.m[0][3] = 0.0f;
    Rx.m[1][0] = 0.0f; Rx.m[1][1] = cosf(AngleXInRadians); Rx.m[1][2] = -sinf(AngleXInRadians);Rx.m[1][3] = 0.0f;
    Rx.m[2][0] = 0.0f; Rx.m[2][1] = sinf(AngleXInRadians); Rx.m[2][2] = cosf(AngleXInRadians);Rx.m[2][3] = 0.0f;
    Rx.m[3][0] = 0.0f; Rx.m[3][1] = 0.0f; Rx.m[3][2] = 0.0f;Rx.m[3][3] = 1.0f;

    return Rx;
}

static void UpdateAngleY(){
    AngleYInDegrees += DeltaYInDegrees;
    
    if(AngleYInDegrees >= 360.0f){
        AngleYInDegrees = 0.0f + DeltaYInDegrees;
    }
    if(AngleYInDegrees <= 0.0f){
        AngleYInDegrees = 360.0f + DeltaYInDegrees;
    }
}
static Matrix4f RotateYMatrix(){

    float AngleYInRadians = (2.0f * M_PI / 360.0f) * AngleYInDegrees;

    Matrix4f Ry;
    Ry.m[0][0] = cosf(AngleYInRadians); Ry.m[0][1] =  0.0f; Ry.m[0][2] = -sinf(AngleYInRadians);Ry.m[0][3] = 0.0f;
    Ry.m[1][0] = 0.0f; Ry.m[1][1] = 1.0f; Ry.m[1][2] = 0.0f;Ry.m[1][3] = 0.0f;
    Ry.m[2][0] = sinf(AngleYInRadians); Ry.m[2][1] = 0.0f; Ry.m[2][2] = cosf(AngleYInRadians);Ry.m[2][3] = 0.0f;
    Ry.m[3][0] = 0.0f; Ry.m[3][1] = 0.0f; Ry.m[3][2] = 0.0f;Ry.m[3][3] = 1.0f;

    return Ry;
}

static Matrix4f ProjectionMatrix(){
    float HalfFovInRadians = (M_PI / 360.0f) * FovInDegrees; 
    float tanHalfFOV = tanf(HalfFovInRadians);
    float inv = 1.0f/tanHalfFOV;

    Matrix4f P(inv,  0.0f, 0.0f, 0.0f,
                0.0f, inv, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f
    );

    // Matrix4f P(1.0f,  0.0f, 0.0f, 0.0f,
    //             0.0f, 1.0f, 0.0f, 0.0f,
    //             0.0f, 0.0f, 1.0f, 0.0f,
    //             0.0f, 0.0f, 0.0f, 1.0f
    // );

    return P;
    
}

void RenderSceneCB(){

    glClear(GL_COLOR_BUFFER_BIT);

    // UpdateScale();
    // Matrix4f S = ScaleMatrix();

    // UpdateLocation();
    Matrix4f T = TranslateMatrix();

    UpdateAngleY();
    Matrix4f Ry = RotateYMatrix();

    Matrix4f P = ProjectionMatrix();

    Matrix4f World = P * T * Ry;

    glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6 * sizeof(float),0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3 * sizeof(float)));

    // glDrawArrays(GL_TRIANGLES,0,6);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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

    SRANDOM;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    int width = 960;
    int height = 960;

    int x = 0;
    int y = 0;

    glutInitWindowSize(width,height);
    glutInitWindowPosition(x,y);

    int window_id = glutCreateWindow("Uniforms");

    std::cout << "window_id : " << window_id << "\n";

    // glEnable(GL_DEPTH_TEST);
    GLenum res = glewInit();

    if(res != GLEW_OK){
        std::cout << "Error: " << glewGetErrorString(res) << "\n";
        return 1;
    }
    else
        std::cout << "glewInit " << res << "\n";

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
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

    glutMainLoop();

    return 0;
}