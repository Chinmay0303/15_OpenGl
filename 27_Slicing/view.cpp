#include "view.h"
#include <GL/freeglut.h>

void View::SetPosition(float x, float y, float z){
    pos_vector.x = x;
    pos_vector.y = y;
    pos_vector.z = z;
}

void View::SetForward(float x, float y, float z){
    forward_vector.x = x;
    forward_vector.y = y;
    forward_vector.z = z;
}

void View::SetUp(float x, float y, float z){
    up_vector.x = x;
    up_vector.y = y;
    up_vector.z = z;
}

Matrix4f View::GetMatrix(){

    Matrix4f ViewMatrix;

    Matrix4f CameraTranslationMatrix = GetCameraTranslationMatrix(-pos_vector.x,-pos_vector.y,-pos_vector.z);

    Matrix4f CameraRotationMatrix = GetCameraRotationMatrix(forward_vector,up_vector);

    ViewMatrix = CameraRotationMatrix * CameraTranslationMatrix;

    return ViewMatrix;
}


Matrix4f View::GetCameraTranslationMatrix(float pos_x, float pos_y, float pos_z)
{
    Matrix4f M;
    M.m[0][0] = 1.0f;   M.m[0][1] = 0.0f;   M.m[0][2] = 0.0f;   M.m[0][3] = pos_x;
    M.m[1][0] = 0.0f;   M.m[1][1] = 1.0f;   M.m[1][2] = 0.0f;   M.m[1][3] = pos_y;
    M.m[2][0] = 0.0f;   M.m[2][1] = 0.0f;   M.m[2][2] = 1.0f;   M.m[2][3] = pos_z;
    M.m[3][0] = 0.0f;   M.m[3][1] = 0.0f;   M.m[3][2] = 0.0f;   M.m[3][3] = 1.0f;

    return M;
}

Matrix4f View::GetCameraRotationMatrix(Vector3f forward_vector, Vector3f up_vector){
    
    const Vector3f forward = Normalize(forward_vector);
    const Vector3f right = Normalize(Cross3(forward, up_vector));
    const Vector3f up = Cross3(right, forward);

    Matrix4f M;

    M.m[0][0] = right.x;   M.m[0][1] = right.y;   M.m[0][2] = right.z;   M.m[0][3] = 0.0f;
    M.m[1][0] = up.x;      M.m[1][1] = up.y;      M.m[1][2] = up.z;      M.m[1][3] = 0.0f;
    M.m[2][0] = -forward.x; M.m[2][1] = -forward.y; M.m[2][2] = -forward.z; M.m[2][3] = 0.0f;
    M.m[3][0] = 0.0f;      M.m[3][1] = 0.0f;      M.m[3][2] = 0.0f;      M.m[3][3] = 1.0f;

    return M;
}

Vector3f View::Normalize(Vector3f vector_){
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

Vector3f View::Cross3(Vector3f vector_1, Vector3f vector_2){

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

float View::getCenterZ(){
    return (float)pos_vector.z;
}

void View::OnKeyboard(unsigned char key){
    switch(key){

        case 'i':
        {
            Vector3f forward = Normalize(forward_vector);

            forward *= speed;
            pos_vector += forward;
        }
        break;

        case 'k':
        {
            Vector3f forward = Normalize(forward_vector);

            forward *= speed;
            pos_vector -= forward;
        }
        break;

        case 'l':
        {
            Vector3f forward = Normalize(forward_vector);
            Vector3f up = Normalize(up_vector);
            Vector3f right = Normalize(Cross3(forward, up));

            pos_vector += right * speed;
        }
            break;

        case 'j':
        {
            Vector3f forward = Normalize(forward_vector);
            Vector3f up = Normalize(up_vector);
            Vector3f right = Normalize(Cross3(forward, up));

            pos_vector -= right * speed;
        }
            break;

        case 'u':
        {
            Vector3f up = Normalize(up_vector);
            pos_vector += up * speed;
        }
        break;

        case 'o':
        {
            Vector3f up = Normalize(up_vector);
            pos_vector -= up * speed;
        }
        break;
    }
}