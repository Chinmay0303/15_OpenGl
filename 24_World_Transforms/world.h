#ifndef WORLD_H
#define WORLD_H

#include "ogldev_math_3d.h"

class World {
    // From local coordinate to world coordinate
    public:
        World(){}

        void SetPosition(float x, float y, float z);
        
        void SetScale(float dx, float dy, float dz);
        void Translate(float dx, float dy, float dz);
        void Rotate(float dx, float dy, float dz);

        Vector3f GetPosition();

        Matrix4f GetMatrix();
    
    private:
        Vector3f pos_vector = Vector3f(0.0f,0.0f,0.0f);
        Vector3f scale_vector = Vector3f(1.0f,1.0f,1.0f);
        Vector3f rot_vector = Vector3f(0.0f,0.0f,0.0f);

        Matrix4f GetScalingMatrix(float ScaleX, float ScaleY, float ScaleZ);
        Matrix4f GetTranslationMatrix(float pos_x, float pos_y, float pos_z);
        Matrix4f GetRotationMatrix(float ang_x, float ang_y, float ang_z);
        Matrix4f GetRotationZMatrix(float ang_z);
        Matrix4f GetRotationXMatrix(float ang_x);
        Matrix4f GetRotationYMatrix(float ang_y);
};


#endif