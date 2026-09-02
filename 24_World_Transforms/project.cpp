#include "project.h"


Matrix4f Project::GetMatrix(){
    
    Matrix4f M;

    float Pi = 3.14159265358979323846f;

    float ar = (float) Width/ (float) Height;

    float HalfFOV_rad = (Pi / 360.0f) * FOV;
    float tanHalfFOV = tanf(HalfFOV_rad);

    float d = 1.0f/tanHalfFOV;
    float A = -(zFar + zNear)/(zFar - zNear);
    float B = -(2.0f*zFar*zNear)/(zFar - zNear);

    M.m[0][0] = d/ar;   M.m[0][1] = 0.0f;    M.m[0][2] = 0.0f;    M.m[0][3] = 0.0f;
    M.m[1][0] = 0.0f;   M.m[1][1] = d;       M.m[1][2] = 0.0f;    M.m[1][3] = 0.0f;
    M.m[2][0] = 0.0f;   M.m[2][1] = 0.0f;    M.m[2][2] = A;       M.m[2][3] = B;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;    M.m[3][2] = -1.0f;    M.m[3][3] = 0.0f;

    return M;

}