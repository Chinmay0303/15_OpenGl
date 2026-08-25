#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint gWorldLocation;

float scale = 0.0f;
float delta = 0.0025f;

int Triangles = 90;
int N = 3 * Triangles; //no. of vertices
float DeltaInDegrees = 360.f/Triangles;

void CreateVertexBuffer(){

    std::cout << "Delta In Degrees: " << DeltaInDegrees << "\n";
    float DeltaInRadians = (2.0f * M_PI/360.0f) * DeltaInDegrees;

    std::vector<Vector3f> Vertices(N);
    Vector3f p0 = Vector3f(0.0f,0.0f,0.0f); //orign
    Vector3f p1 = Vector3f(0.5f,0.0f,0.0f); //right

    Vertices[0] = p0;
    Vertices[1] = p1;

    Matrix3f RotateDot;

    RotateDot.m[0][0] = cosf(DeltaInRadians);RotateDot.m[0][1] = -sinf(DeltaInRadians);RotateDot.m[0][2] = 0.0f;
    RotateDot.m[1][0] = sinf(DeltaInRadians);RotateDot.m[1][1] = cosf(DeltaInRadians);RotateDot.m[1][2] = 0.0f;
    RotateDot.m[2][0] = 0.0f;    RotateDot.m[2][1] = 0.0f;   RotateDot.m[2][2] = 1.0f;

    Vector3f p2 = RotateDot * p1;
    Vertices[2] = p2;

    int i = 3;

    while(i < N){
        
        Vertices[i] = p0; // first vertex: origin
        Vertices[i+1] = Vertices[i-1]; // second vertex: last rotated vertex
        Vertices[i+2] = RotateDot * Vertices[i+1]; // first vertex: third vertex: rotated vertex

        i += 3 ;
    }

    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,Vertices.size() * sizeof(Vector3f),Vertices.data(),GL_STATIC_DRAW);
}

static void UpdateScale(){
    scale += delta;

    if(scale >= 1.5f || scale <= 0.0f){
        delta = -1.0f * delta;
    }
}

static Matrix4f ScalingExample()
{
    Matrix4f Scaling(scale, 0.0f,  0.0f,  0.0f,
                     0.0f,  scale, 0.0f,  0.0f,
                     0.0f,  0.0f,  scale, 0.0f,
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

    glDrawArrays(GL_TRIANGLES,0,N);

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