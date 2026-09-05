#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "ogldev_math_3d.h"
#include "offreader.h"
#include "world.h"
#include "view.h"
#include "project.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

GLuint VBO;
GLuint IBO;
GLuint gWVPLocation;

GLuint gNearColourLocation;
GLuint gFarColourLocation;

GLuint gCenterZLocation;

World ModelWorld;
View ModelView;

GLenum Mode = GL_FILL;
bool CullState = true;

float FOV = 45.0f;
float zNear = 0.1f;
float zFar = 20.0f;

Project ModelProject(FOV,WINDOW_WIDTH,WINDOW_HEIGHT,zNear,zFar);

float scalingFactor = 2.0f;

Matrix4f NormMat;

Vector3f* ModelVertices;
unsigned int* ModelIndex;

std::vector<float> nearColour(3);
std::vector<float> farColour(3);

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

int NumVertices = 0;
int NumIndices = 0;
int NumTriangles = 0;

struct Bounds{
    float minX = INT_MAX;
    float maxX = INT_MIN;

    float minY = INT_MAX;
    float maxY = INT_MIN;

    float minZ = INT_MAX;
    float maxZ = INT_MIN;

    Vector3f center;
    Vector3f range;

    float maxDimension;

    Bounds(){}

    Bounds(Vector3f* VertexArray)
    {
        for(int i = 0; i < NumVertices; i++){
            minX = std::min(minX,VertexArray[i].x);
            minY = std::min(minY,VertexArray[i].y);
            minZ = std::min(minZ,VertexArray[i].z);  

            maxX = std::max(maxX,VertexArray[i].x);
            maxY = std::max(maxY,VertexArray[i].y);
            maxZ = std::max(maxZ,VertexArray[i].z);
        }

        center = Vector3f(
                    (minX + maxX) * 0.5f,
                    (minY + maxY) * 0.5f,
                    (minZ + maxZ) * 0.5f
                );

        range = Vector3f(
                    maxX - minX,
                    maxY - minY,
                    maxZ - minZ
                );

        maxDimension = std::max(range.x,std::max(range.y,range.z));
    }
};

Vector3f randomAxis;

static void UpdateAnimation( Vector3f arbitraryAxis)
{
    if (animationState.paused) {
        return;
    }

    float angleStep = 0.5f;

    if (animationState.x) {
        ModelWorld.Rotate(angleStep, 0.0f, 0.0f);
    }

    if (animationState.y) {
        ModelWorld.Rotate(0.0f, angleStep, 0.0f);
    }

    if (animationState.z) {
        ModelWorld.Rotate(0.0f, 0.0f, angleStep);
    }

    if (animationState.arbitrary) {
        ModelWorld.RotateAroundArbitraryAxis(arbitraryAxis, angleStep);
    }
}


static std::vector<float> getRandomColour(){
    float Max = RAND_MAX;

    float red   = ((float)random() / Max);
    float green = ((float)random() / Max);
    float blue  = ((float)random() / Max);

    std::vector<float> colour = {red,green,blue}; 
        
    return colour;
}

struct ColVertex {
    Vector3f pos;
    Vector3f colour;

    ColVertex(){}

    ColVertex(float x, float y, float z)
    {
        pos = Vector3f(x, y, z);

        std::vector<float> colour_vector = getRandomColour();

        colour.x = colour_vector[0];
        colour.y = colour_vector[1];
        colour.z = colour_vector[2];
        // color = Vector3f(red, 0.0f, 0.0f);
    }
};

static void PrintCoordinates(std::vector<Vector3f> &Vertices){
    if(Vertices.empty()){
        return;
    }
    
    float minXcoord = Vertices[0].x;
    float minYcoord = Vertices[0].y;
    float minZcoord = Vertices[0].z;

    float maxXcoord = Vertices[0].x;
    float maxYcoord = Vertices[0].y;
    float maxZcoord = Vertices[0].z;
    
    for(int i = 0; i < Vertices.size(); i++){

        if(Vertices[i].x < minXcoord){
            minXcoord = Vertices[i].x;
        }
        if(Vertices[i].x > maxXcoord){

            maxXcoord = Vertices[i].x;
        }

        if(Vertices[i].y < minYcoord){
            minYcoord = Vertices[i].y;
        }
        if(Vertices[i].y > maxYcoord){
            maxYcoord = Vertices[i].y;
        }

        if(Vertices[i].z < minZcoord){
            minZcoord = Vertices[i].z;
        }
        if(Vertices[i].z > maxZcoord){
            maxZcoord = Vertices[i].z;
        }
    }
    
    std::cout << "minX: " << minXcoord << " maxX: "<< maxXcoord << "\n";
    std::cout << "minY: " << minYcoord << " maxY: "<< maxYcoord << "\n";
    std::cout << "minZ: " << minZcoord << " maxZ: "<< maxZcoord << "\n";

    float xRange = maxXcoord - minXcoord;
    float yRange = maxYcoord - minYcoord;
    float zRange = maxZcoord - minZcoord;

    float xCenter = (maxXcoord + minXcoord) * 0.5f;
    float yCenter = (maxYcoord + minYcoord) * 0.5f;
    float zCenter = (maxZcoord + minZcoord) * 0.5f;

    std::cout << "xRange: " << xRange << " xCenter: "<< xCenter << "\n";
    std::cout << "yRange: " << yRange << " yCenter: "<< yCenter << "\n";
    std::cout << "zRange: " << zRange << " zCenter: "<< zCenter << "\n";
    std::cout << "\n";
}

static void PrintNormCoordinates(std::vector<Vector3f> &Vertices){
    if(Vertices.empty()){
        return;
    }

    std::vector<Vector4f> Vertices_(NumVertices);

    for(int i = 0; i < Vertices.size(); i++){
        Vector4f NormVertex;
        NormVertex = NormMat * Vector4f(Vertices[i],1.0f);

        Vertices_[i].x = NormVertex.x;
        Vertices_[i].y = NormVertex.y;
        Vertices_[i].z = NormVertex.z;
    }
    
    float minXcoord = Vertices_[0].x;
    float minYcoord = Vertices_[0].y;
    float minZcoord = Vertices[0].z;

    float maxXcoord = Vertices_[0].x;
    float maxYcoord = Vertices_[0].y;
    float maxZcoord = Vertices_[0].z;
    
    for(int i = 0; i < Vertices_.size(); i++){

        if(Vertices_[i].x < minXcoord){
            minXcoord = Vertices_[i].x;
        }
        if(Vertices_[i].x > maxXcoord){

            maxXcoord = Vertices_[i].x;
        }

        if(Vertices_[i].y < minYcoord){
            minYcoord = Vertices_[i].y;
        }
        if(Vertices_[i].y > maxYcoord){
            maxYcoord = Vertices_[i].y;
        }

        if(Vertices_[i].z < minZcoord){
            minZcoord = Vertices_[i].z;
        }
        if(Vertices_[i].z > maxZcoord){
            maxZcoord = Vertices_[i].z;
        }
    }
    
    std::cout << "minX: " << minXcoord << " maxX: "<< maxXcoord << "\n";
    std::cout << "minY: " << minYcoord << " maxY: "<< maxYcoord << "\n";
    std::cout << "minZ: " << minZcoord << " maxZ: "<< maxZcoord << "\n";

    float xRange = maxXcoord - minXcoord;
    float yRange = maxYcoord - minYcoord;
    float zRange = maxZcoord - minZcoord;

    float xCenter = (maxXcoord + minXcoord) * 0.5f;
    float yCenter = (maxYcoord + minYcoord) * 0.5f;
    float zCenter = (maxZcoord + minZcoord) * 0.5f;

    std::cout << "xRange: " << xRange << " xCenter: "<< xCenter << "\n";
    std::cout << "yRange: " << yRange << " yCenter: "<< yCenter << "\n";
    std::cout << "zRange: " << zRange << " zCenter: "<< zCenter << "\n";
    std::cout << "\n";
}


static void CreateVertexBuffer(Vector3f* VertexArray)
{	
    std::cout << "Number of Vertices: " << NumVertices << "\n";

    std::vector<Vector3f> Vertices(NumVertices);

    for(int i = 0; i < NumVertices; i++){
        Vertices[i] = Vector3f(VertexArray[i].x,
            VertexArray[i].y,
            VertexArray[i].z);    
    }

    PrintCoordinates(Vertices);
    PrintNormCoordinates(Vertices);

    std::vector <ColVertex> ColVertices(Vertices.size());

    for(int i = 0; i < Vertices.size(); i++){
        // std::cout << Vertices[i].x << ", " << Vertices[i].y << ", " << Vertices[i].z << "\n";
        ColVertices[i] = ColVertex(Vertices[i].x,Vertices[i].y,Vertices[i].z);
    }

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,ColVertices.size() * sizeof(ColVertex),ColVertices.data(),GL_STATIC_DRAW);

    free(VertexArray);
}

static void CreateIndexBuffer(unsigned int* IndexArray)
{
    std::cout << "Number of Triangles: " << NumTriangles << "\n";
    std::cout << "Number of Indices: " << NumIndices << "\n";

    std::vector<unsigned int> Indices(NumIndices);

    for(int i = 0; i < NumIndices; i++){
        Indices[i] = IndexArray[i];
        // std::cout << IndexArray[i] << ", ";
    }
    
    std::cout << "\n";

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,Indices.size() * sizeof(unsigned int),Indices.data(),GL_STATIC_DRAW);

    free(IndexArray);
}

static void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    UpdateAnimation(randomAxis);
    
    Matrix4f W = ModelWorld.GetMatrix();

    Matrix4f V = ModelView.GetMatrix();

    Matrix4f P = ModelProject.GetMatrix();

    Matrix4f WVP = P * V * W;

    float centerZ = ModelView.getCenterZ();

    glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &WVP.m[0][0]);

    glUniform3fv(gNearColourLocation,1,&nearColour[0]);
    glUniform3fv(gFarColourLocation,1,&farColour[0]);

    glUniform1f(gCenterZLocation,centerZ);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColVertex), (void*)(offsetof(ColVertex,pos)));

    // color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColVertex), (void*)(offsetof(ColVertex,colour)));

    glPolygonMode(GL_FRONT_AND_BACK,Mode);
    glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_INT, 0);
    // glDrawArrays(GL_TRIANGLES,0,3);

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
            ModelWorld.Rotate(rotationStep, 0.0f, 0.0f);
            break;

        case '2':
            ModelWorld.Rotate(-rotationStep, 0.0f, 0.0f);
            break;

        case '3':
            ModelWorld.Rotate(0.0f, rotationStep, 0.0f);
            break;

        case '4':
            ModelWorld.Rotate(0.0f, -rotationStep, 0.0f);
            break;

        case '5':
            ModelWorld.Rotate(0.0f, 0.0f, rotationStep);
            break;

        case '6':
            ModelWorld.Rotate(0.0f, 0.0f, -rotationStep);
            break;

        case '-':
        case '_':
            {
                ModelWorld.ScaleUniform(1.0f/scaleFactor);
            }
            break;

        case '+':
        case '=':
            {
                ModelWorld.ScaleUniform(scaleFactor);
            }
            break;

        // Reset model transform.
        case '0':
            {
                ModelWorld.ResetTransform();
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
            ModelView.OnKeyboard(key);
        }
    }
    glutPostRedisplay();
}


static void SpecialKeyboardCB(int key, int mouse_x, int mouse_y)
{
    float movement = 0.1f;

    switch (key) {
        case GLUT_KEY_LEFT:
            ModelWorld.Translate(-movement, 0.0f, 0.0f);
            break;

        case GLUT_KEY_RIGHT:
            ModelWorld.Translate(movement, 0.0f, 0.0f);
            break;

        case GLUT_KEY_UP:
            ModelWorld.Translate(0.0f, movement, 0.0f);
            break;

        case GLUT_KEY_DOWN:
            ModelWorld.Translate(0.0f, -movement, 0.0f);
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

    gNearColourLocation = glGetUniformLocation(ShaderProgram, "gNearColour");
    if (gNearColourLocation == -1) {
        printf("Error getting uniform location of 'gNearColour'\n");
        exit(1);
    }
    else{
        std::cout << "gNearColourLocation: " << gNearColourLocation << "\n";
    }

    gFarColourLocation = glGetUniformLocation(ShaderProgram, "gFarColour");
    if (gFarColourLocation == -1) {
        printf("Error getting uniform location of 'gFarColour'\n");
        exit(1);
    }
    else{
        std::cout << "gFarColourLocation: " << gFarColourLocation << "\n";
    }

    gCenterZLocation = glGetUniformLocation(ShaderProgram, "gCenterZ");
    if (gCenterZLocation == -1) {
        printf("Error getting uniform location of 'gCenterZ'\n");
        exit(1);
    }
    else{
        std::cout << "gCenterZLocation: " << gCenterZLocation << "\n";
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

    // nearColour = getRandomColour();
    // farColour = getRandomColour();

    nearColour = {0.43137, 0.87059, 0.49804};
    farColour =  {0.90980,0.51765,0.39216};

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    // int width = 960;
    // int height = 960;

    int x = 0;
    int y = 0;
 
    ModelWorld.SetPosition(0.0f,0.0f,0.0f);
    ModelWorld.SetScale(1.0f,1.0f,1.0f);

    ModelView.SetPosition(0.0f, 0.0f, 5.0f);
    ModelView.SetForward(0.0f, 0.0f, -1.0f);
    ModelView.SetUp(0.0f, 1.0f, 0.0f);

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

    OffModel *model = readOffFile(argv[1]);
    // printOffModel(model);

    NumTriangles = getNumTriangles(model);
    NumIndices = NumTriangles * 3;
    NumVertices = getNumVertices(model);

    ModelVertices = getVertices(model);
    ModelIndex = getIndices(model);

    Bounds bounds(ModelVertices);

    float normalizationScale = scalingFactor/bounds.maxDimension;

    ModelWorld.SetNormalization(bounds.center,normalizationScale);
    NormMat = ModelWorld.Normalization_Matrix;

    CreateVertexBuffer(ModelVertices);
    CreateIndexBuffer(ModelIndex);

    FreeOffModel(model);

    CompileShaders();

    // float red = 0.96f;
    // float green = 0.95f;
    // float blue = 0.75f;
    // float alpha = 1.0f;

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 1.0f;

    glClearColor(red,green,blue,alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glutDisplayFunc(RenderSceneCB);

    glutKeyboardFunc(KeyboardCB);
    glutSpecialFunc(SpecialKeyboardCB);

    glutMainLoop();

    return 0;
}