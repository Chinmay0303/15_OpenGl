#ifndef VIEW_H
#define VIEW_H

#include "ogldev_math_3d.h"

class View{
    public:
        View()
            :   pos_vector(0.0f, 0.0f, 0.0f),
                forward_vector(0.0f, 0.0f, -1.0f),
                up_vector(0.0f, 1.0f, 0.0f)
    
        {}

        void SetPosition(float x, float y, float z);
        void SetForward(float x, float y, float z);
        void SetUp(float x, float y, float z);

        void OnKeyboard(unsigned char key);

        Matrix4f GetMatrix();
        float getCenterZ();

    private:
        Vector3f pos_vector;

        Vector3f forward_vector;
        Vector3f up_vector;

        float speed = 0.5f;

        // camera is at origin, looking in -z direction, and its orientation is +y axis

        Matrix4f GetCameraTranslationMatrix(float x, float y, float z);
        Matrix4f GetCameraRotationMatrix(Vector3f forward_vector, Vector3f up_vector);
        Vector3f Normalize (Vector3f vector_);
        Vector3f Cross3(Vector3f vector_1, Vector3f vector_2);
};


#endif