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
            zFar(farPlane),
            Orthographic(false)
        {}

            Matrix4f GetMatrix();

            void ToggleProjection(){
                Orthographic = !Orthographic;
            }
            bool IsOrthographic(){
                return Orthographic;
            }
            void SetViewportSize(float width, float height){
                Width = width;
                Height = height;
            }

    private:
        float FOV;
        float Width;
        float Height;
        float zNear;
        float zFar;

        bool Orthographic;

        Matrix4f GetPerspectiveMatrix();
        Matrix4f GetOrthographicMatrix();


};

#endif
