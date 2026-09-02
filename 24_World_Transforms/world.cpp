#include "world.h"

void World::SetPosition(float x, float y, float z){
    pos_vector.x = x;
    pos_vector.y = y;
    pos_vector.z = z;
}

void World::SetScale(float dx, float dy, float dz){
    scale_vector.x = dx;
    scale_vector.y = dy;
    scale_vector.z = dz;
}

void World::Translate(float dx, float dy, float dz){
    pos_vector.x += dx;
    pos_vector.y += dy;
    pos_vector.z += dz;
}

void World::Rotate(float dx, float dy, float dz){
    rot_vector.x += dx;
    rot_vector.y += dy;
    rot_vector.z += dz;
}

Matrix4f World::GetScalingMatrix(float ScaleX, float ScaleY, float ScaleZ)
{
    Matrix4f M;
    M.m[0][0] = ScaleX; M.m[0][1] = 0.0f;   M.m[0][2] = 0.0f;   M.m[0][3] = 0.0f;
    M.m[1][0] = 0.0f;   M.m[1][1] = ScaleY; M.m[1][2] = 0.0f;   M.m[1][3] = 0.0f;
    M.m[2][0] = 0.0f;   M.m[2][1] = 0.0f;   M.m[2][2] = ScaleZ; M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;   M.m[3][2] = 0.0f;   M.m[3][3] = 1.0f;

    return M;
}

Matrix4f World::GetTranslationMatrix(float pos_x, float pos_y, float pos_z)
{
    Matrix4f M;
    M.m[0][0] = 1.0f;   M.m[0][1] = 0.0f;   M.m[0][2] = 0.0f;   M.m[0][3] = pos_x;
    M.m[1][0] = 0.0f;   M.m[1][1] = 1.0f;   M.m[1][2] = 0.0f;   M.m[1][3] = pos_y;
    M.m[2][0] = 0.0f;   M.m[2][1] = 0.0f;   M.m[2][2] = 1.0f;   M.m[2][3] = pos_z;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;   M.m[3][2] = 0.0f;   M.m[3][3] = 1.0f;

    return M;
}

Matrix4f World::GetRotationMatrix(float ang_x, float ang_y, float ang_z)
{

    float ang_x_rad = (2*M_PI/360.0f) * ang_x;
    float ang_y_rad = (2*M_PI/360.0f) * ang_y;
    float ang_z_rad = (2*M_PI/360.0f) * ang_z;

    Matrix4f Rx, Ry, Rz;

    Rx = GetRotationXMatrix(ang_x_rad);
    Ry = GetRotationYMatrix(ang_y_rad);
    Rz = GetRotationZMatrix(ang_z_rad);

    Matrix4f R = Rz * Ry * Rx;

    return R;
}

Matrix4f World::GetRotationZMatrix(float ang_z){
    
    Matrix4f M;
    M.m[0][0] = cosf(ang_z); M.m[0][1] = -sinf(ang_z);   M.m[0][2] = 0.0f;   M.m[0][3] = 0.0f;
    M.m[1][0] = sinf(ang_z);   M.m[1][1] = cosf(ang_z); M.m[1][2] = 0.0f;   M.m[1][3] = 0.0f;
    M.m[2][0] = 0.0f;   M.m[2][1] = 0.0f;   M.m[2][2] = 1.0f; M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;   M.m[3][2] = 0.0f;   M.m[3][3] = 1.0f;

    return M;
}

Matrix4f World::GetRotationXMatrix(float ang_x){
    
    Matrix4f M;
    M.m[0][0] = 1.0f;   M.m[0][1] = 0.0f;           M.m[0][2] = 0.0f;           M.m[0][3] = 0.0f;
    M.m[1][0] = 0.0f;   M.m[1][1] = cosf(ang_x);    M.m[1][2] = -sinf(ang_x);   M.m[1][3] = 0.0f;
    M.m[2][0] = 0.0f;   M.m[2][1] = sinf(ang_x);    M.m[2][2] = cosf(ang_x);    M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;           M.m[3][2] = 0.0f;           M.m[3][3] = 1.0f;

    return M;
}

Matrix4f World::GetRotationYMatrix(float ang_y){
    
    Matrix4f M;
    M.m[0][0] = cosf(ang_y);    M.m[0][1] = 0.0f;   M.m[0][2] = sinf(ang_y);    M.m[0][3] = 0.0f;
    M.m[1][0] = 0.0f;           M.m[1][1] = 1.0f;   M.m[1][2] = 0.0f;           M.m[1][3] = 0.0f;
    M.m[2][0] = -sinf(ang_y);    M.m[2][1] = 0.0f;   M.m[2][2] = cosf(ang_y);    M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f;           M.m[3][1] = 0.0f;   M.m[3][2] = 0.0f;           M.m[3][3] = 1.0f;

    return M;
}

Matrix4f World::GetMatrix(){
    Matrix4f S = GetScalingMatrix(scale_vector.x,scale_vector.y,scale_vector.z);

    Matrix4f T = GetTranslationMatrix(pos_vector.x,pos_vector.y,pos_vector.z);

    Matrix4f R = GetRotationMatrix(rot_vector.x,rot_vector.y,rot_vector.z);

    Matrix4f WorldTransform = T * R * S;

    return WorldTransform;
}

Vector3f World::GetPosition(){
    Vector3f position = pos_vector;

    return position;
}