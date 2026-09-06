#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include "ogldev_math_3d.h"
#include "offreader.h"
#include "world.h"
#include "view.h"
#include "project.h"

float WINDOW_WIDTH  = 960.0f;
float WINDOW_HEIGHT = 540.0f;

float WindowPositionX = 0.0f;
float WindowPositionY = 0.0f;

bool isFullScreen = false;

GLuint ShaderProgram;

GLuint VBO;
GLuint IBO;
GLuint VAO;
GLint gWorldLocation;

GLint gViewLocation;
GLint gProjectionLocation;

GLint gNearColourLocation;
GLint gFarColourLocation;

GLint gCenterZLocation;

GLint gLightPositionViewLocation;
GLint gLightAmbientLocation;
GLint gLightDiffuseLocation;

GLint gMaterialAmbientLocation;
GLint gMaterialDiffuseLocation;

GLint gAmbientEnabledLocation;
GLint gDiffuseEnabledLocation;

GLint gLightSpecularLocation;
GLint gMaterialSpecularLocation;
GLint gMaterialShininessLocation;

GLint gSpecularEnabledLocation;

World ModelWorld;
View ModelView;

char * theProgramTitle = "Assignment 1";

const int ANIMATION_DELAY = 20; /* milliseconds between rendering */

GLenum Mode = GL_FILL;
bool CullState = true;

float FOV = 45.0f;
float zNear = 0.1f;
float zFar = 20.0f;

Project ModelProject(FOV,WINDOW_WIDTH,WINDOW_HEIGHT,zNear,zFar);

float scalingFactor = 2.0f;

Matrix4f NormMat;

Vector3f* ModelVertices;
unsigned int* ModelIndices;


struct AnimationState {
    bool x = false;
    bool y = false;
    bool z = false;
    bool arbitrary = false;
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
    
    float angleStep = 1.5f;
    
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

Vector3f nearColour;
Vector3f farColour;

static Vector3f getRandomColour(){
    float Max = RAND_MAX;
    
    float red   = ((float)rand() / Max);
    float green = ((float)rand() / Max);
    float blue  = ((float)rand() / Max);
    
    return Vector3f(red,green,blue);
}

struct ColVertex {
    Vector3f pos;
    Vector3f colour;
    Vector3f normal;

    ColVertex(Vector3f vertexPosition,
        Vector3f vertexColour,
        Vector3f vertexNormal)
        : pos(vertexPosition),
          colour(vertexColour),
          normal(vertexNormal)
        {}

    // ColVertex(float x, float y, float z)
    // {
    //     pos = Vector3f(x, y, z);

    //     colour = getRandomColour();
    //     // color = Vector3f(1.0f, 0.0f, 0.0f);
    // }
};

static Vector3f Normalize(Vector3f vector_){
    Vector3f vector_norm;

    float x = vector_.x;
    float y = vector_.y;
    float z = vector_.z;

    float length = sqrtf(x * x + y * y + z * z);

    if (length <= 0.000001f) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    vector_norm.x = vector_.x/length;
    vector_norm.y = vector_.y/length;
    vector_norm.z = vector_.z/length;

    return vector_norm;
}

static Vector3f Cross3(Vector3f vector_1, Vector3f vector_2){

    Vector3f cross_product_vector;

    float x1 = vector_1.x;
    float y1 = vector_1.y;
    float z1 = vector_1.z;

    float x2 = vector_2.x;
    float y2 = vector_2.y;
    float z2 = vector_2.z;

    cross_product_vector.x = y1*z2 - y2*z1;
    cross_product_vector.y = -(x1*z2 - x2*z1);
    cross_product_vector.z = x1*y2 - x2*y1;

    return cross_product_vector;
}

static std::vector<Vector3f> CalculateVertexNormals(Vector3f* vertices, unsigned int* indices){
    
    std::vector<Vector3f> normals(NumVertices, Vector3f(0.0f, 0.0f, 0.0f));

    for (int i = 0; i < NumIndices; i += 3) {
        const unsigned int index0 = indices[i];
        const unsigned int index1 = indices[i + 1];
        const unsigned int index2 = indices[i + 2];

        const Vector3f& p0 = vertices[index0];
        const Vector3f& p1 = vertices[index1];
        const Vector3f& p2 = vertices[index2];

        const Vector3f edge1 = p1 - p0;
        const Vector3f edge2 = p2 - p0;

        const Vector3f faceNormal = Normalize(Cross3(edge1, edge2));

        normals[index0] += faceNormal;
        normals[index1] += faceNormal;
        normals[index2] += faceNormal;
    }

    for (Vector3f& normal : normals) {
        normal = Normalize(normal);
    }

    return normals;
}

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

    std::vector<Vector4f> Vertices_(Vertices.size());

    for(int i = 0; i < Vertices.size(); i++){
        Vector4f NormVertex;
        NormVertex = NormMat * Vector4f(Vertices[i],1.0f);

        Vertices_[i].x = NormVertex.x;
        Vertices_[i].y = NormVertex.y;
        Vertices_[i].z = NormVertex.z;
    }
    
    float minXcoord = Vertices_[0].x;
    float minYcoord = Vertices_[0].y;
    float minZcoord = Vertices_[0].z;

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

struct Light {
    Vector3f position;
    Vector3f ambient;
    Vector3f diffuse;
    Vector3f specular;
};

struct Material {
    Vector3f ambient;
    Vector3f diffuse;
    Vector3f specular;

    float shininess;
};

static Light SceneLight;
static Material ModelMaterial;

static bool AmbientEnabled = false;
static bool DiffuseEnabled = false;
static bool SpecularEnabled = false;

/* post: compute frames per second and display in window's title bar */
void computeFPS() {
	static int frameCount = 0;
	static int lastFrameTime = 0;
	static char * title = NULL;
	int currentTime;

	if (!title)
		title = (char*) malloc((strlen(theProgramTitle) + 20) * sizeof (char));
	frameCount++;
	currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
	if (currentTime - lastFrameTime > 1000) {
		sprintf(title, "%s [ FPS: %4.2f ]",
			theProgramTitle,
			frameCount * 1000.0 / (currentTime - lastFrameTime));
		glutSetWindowTitle(title);
		lastFrameTime = currentTime;
		frameCount = 0;
	}
}

static void ComputeFPS()
{
    static int frameCount = 0;
    static int previousTime = 0;
    static char title[128];

    const int currentTime =
        glutGet(GLUT_ELAPSED_TIME);

    ++frameCount;

    const int elapsed =
        currentTime - previousTime;

    if (elapsed >= 1000) {
        const double fps =
            frameCount * 1000.0 / elapsed;

        std::snprintf(
            title,
            sizeof(title),
            "%s [FPS: %.2f]",
            theProgramTitle,
            fps);

        glutSetWindowTitle(title);

        frameCount = 0;
        previousTime = currentTime;
    }
}

static void CreateVertexBuffer(Vector3f* VertexArray, unsigned int* IndexArray)
{	
    std::cout << "Number of Vertices: " << NumVertices << "\n";

    std::vector<Vector3f> normals =CalculateVertexNormals(VertexArray, IndexArray);

    std::vector<Vector3f> Vertices(NumVertices);

    std::vector<ColVertex> colVertices;
    colVertices.reserve(NumVertices);

    for(int i = 0; i < NumVertices; i++){
        colVertices.emplace_back(VertexArray[i],getRandomColour(),normals[i]);   
        Vertices[i] = VertexArray[i]; 
    }

    PrintCoordinates(Vertices);
    PrintNormCoordinates(Vertices);

    // std::vector <ColVertex> ColVertices(Vertices.size());

    // for(int i = 0; i < Vertices.size(); i++){
    //     // std::cout << Vertices[i].x << ", " << Vertices[i].y << ", " << Vertices[i].z << "\n";
    //     ColVertices[i] = ColVertex(Vertices[i].x,Vertices[i].y,Vertices[i].z);
    // }

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,colVertices.size() * sizeof(ColVertex),colVertices.data(),GL_STATIC_DRAW);

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

    // Matrix4f WVP = P * V * W;

    Vector4f lightPositionView = V * Vector4f(SceneLight.position.x,
                                              SceneLight.position.y,
                                              SceneLight.position.z,
                                              1.0f);


    float centerZ = ModelView.getCenterZ();

    glUseProgram(ShaderProgram);

    glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &W.m[0][0]);
    // glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, &WVP.m[0][0]);

    glUniformMatrix4fv(gViewLocation, 1, GL_TRUE, &V.m[0][0]);

    glUniformMatrix4fv(gProjectionLocation, 1, GL_TRUE, &P.m[0][0]);

    glUniform3f(gNearColourLocation,nearColour.x,nearColour.y,nearColour.z);

    glUniform3f(gFarColourLocation,farColour.x,farColour.y,farColour.z);

    glUniform1f(gCenterZLocation,centerZ);

    // glUniform3f(gLightPositionLocation,SceneLight.position.x,
    //             SceneLight.position.y,SceneLight.position.z);

    glUniform3f(gLightPositionViewLocation,lightPositionView.x,
                lightPositionView.y,lightPositionView.z);

    glUniform3f(gLightAmbientLocation,SceneLight.ambient.x,
                SceneLight.ambient.y,SceneLight.ambient.z);

    glUniform3f(gLightDiffuseLocation,SceneLight.diffuse.x,
                SceneLight.diffuse.y,SceneLight.diffuse.z);

    glUniform3f(gMaterialAmbientLocation,ModelMaterial.ambient.x,
                ModelMaterial.ambient.y,ModelMaterial.ambient.z);

    glUniform3f(gMaterialDiffuseLocation,ModelMaterial.diffuse.x,
                ModelMaterial.diffuse.y,ModelMaterial.diffuse.z);

    glUniform3f(gLightSpecularLocation,SceneLight.specular.x,
                SceneLight.specular.y,SceneLight.specular.z);

    glUniform3f(gMaterialSpecularLocation,ModelMaterial.specular.x,
                ModelMaterial.specular.y,ModelMaterial.specular.z);


    glUniform1i(gAmbientEnabledLocation,AmbientEnabled ? GL_TRUE : GL_FALSE);

    glUniform1i(gDiffuseEnabledLocation,DiffuseEnabled ? GL_TRUE : GL_FALSE);

    glUniform1i(gSpecularEnabledLocation,SpecularEnabled ? GL_TRUE : GL_FALSE);

    glUniform1f(gMaterialShininessLocation,ModelMaterial.shininess);

    glBindVertexArray(VAO);

    glPolygonMode(GL_FRONT_AND_BACK,Mode);
    glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    glutSwapBuffers();
    // glutPostRedisplay();
    computeFPS();
    // ComputeFPS();
}

/* pre:  glut window has been resized
 */
static void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	if (!isFullScreen) {
		WINDOW_WIDTH = width;
		WINDOW_HEIGHT = height;
	}
	// update scene based on new aspect ratio....
}

/* pre:  glut window is not doing anything else
   post: scene is updated and re-rendered if necessary */
static void onIdle() {
	static int oldTime = 0;
	if (!animationState.paused) {
		int currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
		/* Ensures fairly constant framerate */
		if (currentTime - oldTime > ANIMATION_DELAY) {
			// do animation....
			// rotation += 0.001;

			oldTime = currentTime;
			/* compute the frame rate */
			// computeFPS();
			/* notify window it has to be repainted */
			glutPostRedisplay();
		}
	}
}

/* pre:  glut window has just been iconified or restored 
   post: if window is visible, animate model, otherwise don't bother */
static void onVisible(int state) {
	if (state == GLUT_VISIBLE) {
		/* tell glut to show model again */
		glutIdleFunc(onIdle);
	} else {
		glutIdleFunc(NULL);
	}
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
        
        case 'a':
        case 'A':
            AmbientEnabled = !AmbientEnabled;
            break;

        case 'd':
        case 'D':
            DiffuseEnabled = !DiffuseEnabled;
            break;

        case 's':
        case 'S':
            SpecularEnabled = !SpecularEnabled;
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

        case GLUT_KEY_F1:
			isFullScreen = !isFullScreen;
			if (isFullScreen) {
				WindowPositionX = glutGet((GLenum) (GLUT_WINDOW_X));
				WindowPositionY = glutGet((GLenum) (GLUT_WINDOW_Y));
				glutFullScreen();
			} else {
				glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
				glutPositionWindow(WindowPositionX, WindowPositionY);
			}
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

static void CheckUniformLocation(GLint location, const char* name)
{
    if (location == -1) {
        std::cerr
            << "Error getting uniform location of '"
            << name << "'\n";

        std::exit(EXIT_FAILURE);
    }
}

static void CompileShaders()
{
    ShaderProgram = glCreateProgram();

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
    CheckUniformLocation(gWorldLocation,"gWorldLocation");

    // gWVPLocation = glGetUniformLocation(ShaderProgram, "gWVP");
    // CheckUniformLocation(gWVPLocation,"gWVPLocation");

    gNearColourLocation = glGetUniformLocation(ShaderProgram, "gNearColour");
    CheckUniformLocation(gNearColourLocation,"gNearColourLocation");

    gFarColourLocation = glGetUniformLocation(ShaderProgram, "gFarColour");
    CheckUniformLocation(gFarColourLocation,"gFarColourLocation");

    gCenterZLocation = glGetUniformLocation(ShaderProgram, "gCenterZ");
    CheckUniformLocation(gCenterZLocation,"gCenterZLocation");

    // gLightPositionLocation = glGetUniformLocation(ShaderProgram,"gLightPosition");
    // CheckUniformLocation(gLightPositionLocation,"gLightPositionLocation");

    gLightAmbientLocation = glGetUniformLocation(ShaderProgram,"gLightAmbient");
    CheckUniformLocation(gLightAmbientLocation,"gLightAmbientLocation");

    gLightDiffuseLocation = glGetUniformLocation(ShaderProgram,"gLightDiffuse");
    CheckUniformLocation(gLightDiffuseLocation,"gLightDiffuseLocation");

    gMaterialAmbientLocation = glGetUniformLocation(ShaderProgram,"gMaterialAmbient");
    CheckUniformLocation(gMaterialAmbientLocation,"gMaterialAmbientLocation");

    gMaterialDiffuseLocation = glGetUniformLocation(ShaderProgram,"gMaterialDiffuse");
    CheckUniformLocation(gMaterialDiffuseLocation,"gMaterialDiffuseLocation");

    gAmbientEnabledLocation = glGetUniformLocation(ShaderProgram,"gAmbientEnabled");
    CheckUniformLocation(gAmbientEnabledLocation,"gAmbientEnabledLocation");

    gDiffuseEnabledLocation = glGetUniformLocation(ShaderProgram,"gDiffuseEnabled");
    CheckUniformLocation(gDiffuseEnabledLocation,"gDiffuseEnabledLocation");

    gLightSpecularLocation = glGetUniformLocation(ShaderProgram,"gLightSpecular");
    CheckUniformLocation(gLightSpecularLocation,"gLightSpecularLocation");

    gMaterialSpecularLocation = glGetUniformLocation(ShaderProgram,"gMaterialSpecular");
    CheckUniformLocation(gMaterialSpecularLocation,"gMaterialSpecularLocation");

    gMaterialShininessLocation = glGetUniformLocation(ShaderProgram,"gMaterialShininess");
    CheckUniformLocation(gMaterialShininessLocation,"gMaterialShininessLocation");

    gSpecularEnabledLocation = glGetUniformLocation(ShaderProgram,"gSpecularEnabled");
    CheckUniformLocation(gSpecularEnabledLocation,"gSpecularEnabledLocation");

    gViewLocation = glGetUniformLocation(ShaderProgram,"gView");
    CheckUniformLocation(gViewLocation,"gViewLocation");

    gProjectionLocation = glGetUniformLocation(ShaderProgram,"gProjection");
    CheckUniformLocation(gProjectionLocation,"gProjectionLocation");

    gLightPositionViewLocation = glGetUniformLocation(ShaderProgram,"gLightPositionView");
    CheckUniformLocation(gLightPositionViewLocation,"gLightPositionViewLocation");

    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glUseProgram(ShaderProgram);
}

static void ConfigureVertexAttributes()
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(ColVertex),
        reinterpret_cast<void*>(offsetof(ColVertex, pos)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(ColVertex),
        reinterpret_cast<void*>(offsetof(ColVertex, colour)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(ColVertex),
        reinterpret_cast<void*>(offsetof(ColVertex, normal)));

    glBindVertexArray(0);
}

int main(int argc, char** argv){

    srand(time(0));

    randomAxis = CreateRandomAxis();

    // nearColour = getRandomColour();
    // farColour = getRandomColour();

    // nearColour = Vector3f(0.43137, 0.87059, 0.49804);
    // farColour =  Vector3f(0.90980,0.51765,0.39216);

    nearColour = Vector3f(1.0, 0.2, 0.0);
    farColour =  Vector3f(0.2, 1.0, 0.0);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    
    // int width = 960;
    // int height = 960;
    
    // int x = 0;
    // int y = 0;
    
    ModelWorld.SetPosition(0.0f,0.0f,0.0f);
    ModelWorld.SetScale(1.0f,1.0f,1.0f);

    ModelView.SetPosition(0.0f, 0.0f, 5.0f);
    ModelView.SetForward(0.0f, 0.0f, -1.0f);
    ModelView.SetUp(0.0f, 1.0f, 0.0f);
    
    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutInitWindowPosition(WindowPositionX,WindowPositionY);
    
    // int window_id = glutCreateWindow("Uniforms");
    int window_id = glutCreateWindow(theProgramTitle);

    glutIdleFunc(onIdle);
    glutVisibilityFunc(onVisible);

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
    
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);

    OffModel *model = readOffFile(argv[1]);
    // printOffModel(model);

    if (model == nullptr) {
        std::cerr << "Could not load model: "
                  << argv[1] << '\n';
        return 1;
    }

    NumTriangles = getNumTriangles(model);
    NumIndices = NumTriangles * 3;
    NumVertices = getNumVertices(model);

    ModelVertices = getVertices(model);
    ModelIndices = getIndices(model);

    SceneLight.position = Vector3f(3.0f, 3.0f, 3.0f);
    SceneLight.ambient = Vector3f(1.0f, 1.0f, 1.0f);
    SceneLight.diffuse = Vector3f(1.0f, 1.0f, 1.0f);
    SceneLight.specular = Vector3f(0.3f,0.3f,0.3f);

    ModelMaterial.ambient = Vector3f(0.25f, 0.25f, 0.25f);
    ModelMaterial.diffuse = Vector3f(0.8f, 0.8f, 0.4f);
    ModelMaterial.specular = Vector3f(0.5f, 0.5f, 0.5f);
    ModelMaterial.shininess = 16.0f;
    // 8       broad, dull highlight
    // 32      moderate highlight
    // 128     small, sharp highlight
    // 256     very sharp highlight

    Bounds bounds(ModelVertices);

    float normalizationScale = scalingFactor/bounds.maxDimension;

    ModelWorld.SetNormalization(bounds.center,normalizationScale);
    NormMat = ModelWorld.Normalization_Matrix;

    CreateVertexBuffer(ModelVertices,ModelIndices);
    CreateIndexBuffer(ModelIndices);

    ConfigureVertexAttributes();

    glBindVertexArray(0);

    FreeOffModel(model);

    CompileShaders();

    float red = 0.96f;
    float green = 0.95f;
    float blue = 0.75f;
    float alpha = 1.0f;

    // float red = 0.0f;
    // float green = 0.0f;
    // float blue = 0.0f;
    // float alpha = 1.0f;

    glClearColor(red,green,blue,alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    glutDisplayFunc(RenderSceneCB);

    glutKeyboardFunc(KeyboardCB);
    glutSpecialFunc(SpecialKeyboardCB);

    glutMainLoop();

    return 0;
}
