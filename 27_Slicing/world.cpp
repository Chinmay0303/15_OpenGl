#include "world.h"

void World::SetNormalization(Vector3f modelCenter, float modelScale){

    Matrix4f T = GetTranslationMatrix(-modelCenter.x,-modelCenter.y,-modelCenter.z);

    Matrix4f S = GetScalingMatrix(modelScale,modelScale,modelScale);

    Normalization_Matrix = S * T;
}
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

    Matrix4f WorldTransform = T * R * Arbitrary_rotation * S * Normalization_Matrix;

    return WorldTransform;
}

Vector3f World::GetPosition(){
    Vector3f position = pos_vector;

    return position;
}

void World::ScaleUniform(float factor){
    
    if(factor <= 0.0f){
        return;
    }

    scale_vector.x *= factor;
    scale_vector.y *= factor;
    scale_vector.z *= factor;
}

void World::ResetTransform(){

    pos_vector = Vector3f(0.0f,0.0f,0.0f);
    scale_vector = Vector3f(1.0f,1.0f,1.0f);
    rot_vector = Vector3f(0.0f,0.0f,0.0f);

    Arbitrary_rotation = IdentityMatrix();
}

Matrix4f World::IdentityMatrix()
{
    Matrix4f M;

    M.m[0][0] = 1.0f; M.m[0][1] = 0.0f; M.m[0][2] = 0.0f; M.m[0][3] = 0.0f;
    M.m[1][0] = 0.0f; M.m[1][1] = 1.0f; M.m[1][2] = 0.0f; M.m[1][3] = 0.0f;
    M.m[2][0] = 0.0f; M.m[2][1] = 0.0f; M.m[2][2] = 1.0f; M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f; M.m[3][1] = 0.0f; M.m[3][2] = 0.0f; M.m[3][3] = 1.0f;

    return M;
}

Vector3f World::Normalize(Vector3f vector_){
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

Matrix4f World::GetArbitraryRotationMatrix(Vector3f axis,float angleDegrees)
{
    float Pi = 3.14159265358979323846f;
    float Epsilon = 0.000001f;

    Vector3f a = Normalize(axis);

    float axisLength = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);

    if (axisLength <= Epsilon) {
        return IdentityMatrix();
    }

    float d = sqrtf(
        a.y * a.y + a.z * a.z);

    float theta = angleDegrees * Pi / 180.0f;

    Matrix4f Rx = IdentityMatrix();
    Matrix4f Ry = IdentityMatrix();
    Matrix4f Rz = IdentityMatrix();

    if(d >= Epsilon) {
        Rx.m[1][1] =  a.z / d;
        Rx.m[1][2] = -a.y / d;
        Rx.m[2][1] =  a.y / d;
        Rx.m[2][2] =  a.z / d;
    }
    
    // Matrix4f Ry;
    Ry.m[0][0] = d;       Ry.m[0][1] = 0.0f; Ry.m[0][2] = -a.x;  Ry.m[0][3] = 0.0f;
    Ry.m[1][0] = 0.0f;   Ry.m[1][1] = 1.0f; Ry.m[1][2] = 0.0f;  Ry.m[1][3] = 0.0f;
    Ry.m[2][0] = a.x;     Ry.m[2][1] = 0.0f; Ry.m[2][2] = d;     Ry.m[2][3] = 0.0f;
    Ry.m[3][0] = 0.0f;   Ry.m[3][1] = 0.0f; Ry.m[3][2] = 0.0f;  Ry.m[3][3] = 1.0f;

    // Matrix4f Rz;
    Rz.m[0][0] = cosf(theta);     Rz.m[0][1] = -sinf(theta);    Rz.m[0][2] = 0.0f; Rz.m[0][3] = 0.0f;
    Rz.m[1][0] = sinf(theta);     Rz.m[1][1] = cosf(theta);     Rz.m[1][2] = 0.0f; Rz.m[1][3] = 0.0f;
    Rz.m[2][0] = 0.0f; Rz.m[2][1] = 0.0f;  Rz.m[2][2] = 1.0f; Rz.m[2][3] = 0.0f;
    Rz.m[3][0] = 0.0f; Rz.m[3][1] = 0.0f;  Rz.m[3][2] = 0.0f; Rz.m[3][3] = 1.0f;

    // For orthonormal alignment matrices:
    // Rx(-θx) = transpose(Rx)
    // Ry(-θy) = transpose(Ry)
    const Matrix4f RxInverse = Rx.Transpose();
    const Matrix4f RyInverse = Ry.Transpose();

    return RxInverse * RyInverse * Rz * Ry * Rx;
}

void World::RotateAroundArbitraryAxis(Vector3f axis, float angleDegrees)
{
    Vector3f a = Normalize(axis);

    if (a.x == 0.0f &&
        a.y == 0.0f &&
        a.z == 0.0f) {
        return;
    }

    Matrix4f Increment = GetArbitraryRotationMatrix(a,angleDegrees);

    // Post-multiplication applies the increment in model/local space.
    Arbitrary_rotation = Arbitrary_rotation * Increment;
}