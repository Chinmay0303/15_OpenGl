#ifndef PROJECT_H
#define PROJECT_H

#include "ogldev_math_3d.h"

class Project {
    public:
        Project(float fov, float width, float height,
                float nearPlane, float farPlane)
            : FOV(fov),
            Width(width),
            Height(height),
            zNear(nearPlane),
            zFar(farPlane)
        {}

            Matrix4f GetMatrix();
    private:
        float FOV;
        float Width;
        float Height;
        float zNear;
        float zFar;

};

#endif
