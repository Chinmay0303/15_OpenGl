#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"

GLuint VBO;
GLuint gWorldLocation;
int Lines = 360;
int N = 2 * Lines;

void CreateVertexBuffer(){

    std::vector<Vector3f> Vertices(N);
    Vector3f p0 = Vector3f(0.0f,0.0f,0.0f); //origin
    Vector3f p1 = Vector3f(0.5f,0.0f,0.0f); //right

    Vertices[0] = p0;
    Vertices[1] = p1;

    int i = 2;
    // float Delta = 360.0f/N;
    float Delta = 2 * 360.0f / N;
    float AngleInDegrees = 0.0f;

    std::cout << "Delta: " << Delta << "\n";
    
    Matrix3f RotateDot;

    while(i < N){
        AngleInDegrees += Delta;
        float AngleInRadians = 2.0f * M_PI * AngleInDegrees/360.0f;

        // to rotate around z axis
        RotateDot.m[0][0] = cosf(AngleInRadians); RotateDot.m[0][1] = -sinf(AngleInRadians); RotateDot.m[0][2] = 0.0f;
        RotateDot.m[1][0] = sinf(AngleInRadians); RotateDot.m[1][1] = cosf(AngleInRadians); RotateDot.m[1][2] = 0.0f; 
        RotateDot.m[2][0] = 0.0f; RotateDot.m[2][1] = 0.0f; RotateDot.m[2][2] = 1.0f;

        Vertices[i] = p0; // first vertex: origin
        Vector3f p2 = RotateDot * p1;
        Vertices[i+1] = p2; // second vertex: rotated vertex

        i += 2;
    }

    // for(int i = 0; i < N; i++){
    //     std::cout << i << "\n";
    //     for(int j = 0; j < 3; j++){
    //         std::cout << Vertices[i][j] << ", ";
    //     }
    //     std::cout << "\n";
    // }


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

    static int lines = 0;
    int step = 2;

    int start = std::max(0,lines-N);
    int end = std::min(N,lines);
    int count = end - start;

    if(count > 0){
        glDrawArrays(GL_LINES,start,count);
    }
    else{
        std::cout << "count: " << count << " start: "<< start << " end: " << end << "\n";
    }

    lines += step;

    if(lines > 2*N){
        lines = 0;
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