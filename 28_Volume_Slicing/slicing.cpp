#include "slicing.h"

#include <cmath>
#include <algorithm>
#include <iostream>

Slicing::Slicing()
{
    position[0] = 0.5f;
    position[1] = 0.5f;
    position[2] = 0.5f;

    visible[0] = true;
    visible[1] = true;
    visible[2] = true;

    dirty[0] = true;
    dirty[1] = true;
    dirty[2] = true;
}

bool Slicing::HandleKey(unsigned char key, const Volume& volume)
{
    auto stepAxis = [&](int axis, int n, int direction) {
        float step = (n > 1) ? static_cast<float>(direction * stepVoxels) / (n - 1) : 0.0f;
        position[axis] = std::clamp(position[axis] + step, 0.0f, 1.0f);
        dirty[axis] = true;
        return true;
    };

    switch (key) {
        case 'x': return stepAxis(0, volume.nx, -1);
        case 'X': return stepAxis(0, volume.nx, +1);
        case 'y': return stepAxis(1, volume.ny, -1);
        case 'Y': return stepAxis(1, volume.ny, +1);
        case 'z': return stepAxis(2, volume.nz, -1);
        case 'Z': return stepAxis(2, volume.nz, +1);
        case '7': visible[0] = !visible[0]; return true;
        case '8': visible[1] = !visible[1]; return true;
        case '9': visible[2] = !visible[2]; return true;
        default:  return false;
    }
}

bool Slicing::IsDirty(SliceAxis axis) const
{
    return dirty[static_cast<int>(axis)];
}

void Slicing::ClearDirty(SliceAxis axis)
{
    dirty[static_cast<int>(axis)] = false;
}

float Slicing::GetPosition(SliceAxis axis) const
{
    return position[static_cast<int>(axis)];
}

void Slicing::SetPosition(SliceAxis axis, float t)
{
    position[static_cast<int>(axis)] = std::clamp(t, 0.0f, 1.0f);
    dirty[static_cast<int>(axis)] = true;
}

bool Slicing::IsVisible(SliceAxis axis) const
{
    return visible[static_cast<int>(axis)];
}

void Slicing::SetVisible(SliceAxis axis, bool v)
{
    visible[static_cast<int>(axis)] = v;
}

int Slicing::GetSliceIndex(SliceAxis axis, const Volume& volume) const
{
    int idx = static_cast<int>(axis);
    int n = 0;
    if (idx == 0) n = volume.nx;
    else if (idx == 1) n = volume.ny;
    else n = volume.nz;

    if (n <= 1) return 0;
    return static_cast<int>(std::round(position[idx] * (n - 1)));
}

SliceImage Slicing::BuildSliceImage(const Volume& volume, const Colourmap& colormap, SliceAxis axis) const
{
    SliceImage img;
    float fixedCoord = 0.0f;

    switch (axis) {
        case SliceAxis::X: // YZ plane: fixed x, u=y, v=z
            img.width = volume.ny;
            img.height = volume.nz;
            fixedCoord = position[0] * (volume.nx - 1.0f);
            break;
        case SliceAxis::Y: // ZX plane: fixed y, u=z, v=x
            img.width = volume.nz;
            img.height = volume.nx;
            fixedCoord = position[1] * (volume.ny - 1.0f);
            break;
        case SliceAxis::Z: // XY plane: fixed z, u=x, v=y
            img.width = volume.nx;
            img.height = volume.ny;
            fixedCoord = position[2] * (volume.nz - 1.0f);
            break;
    }

    img.pixels.resize(img.width * img.height * 4);
    unsigned char* p = img.pixels.data();

    for (int j = 0; j < img.height; ++j) {
        float vj = static_cast<float>(j);
        for (int i = 0; i < img.width; ++i) {
            float ui = static_cast<float>(i);
            float x = (axis == SliceAxis::X) ? fixedCoord : ((axis == SliceAxis::Y) ? vj : ui);
            float y = (axis == SliceAxis::Y) ? fixedCoord : ((axis == SliceAxis::X) ? ui : vj);
            float z = (axis == SliceAxis::Z) ? fixedCoord : ((axis == SliceAxis::X) ? vj : ui);

            float s = volume.Sample(x, y, z);
            Vector3f rgb = colormap.Lookup(s);
            *p++ = static_cast<unsigned char>(std::clamp(std::round(rgb.x * 255.0f), 0.0f, 255.0f));
            *p++ = static_cast<unsigned char>(std::clamp(std::round(rgb.y * 255.0f), 0.0f, 255.0f));
            *p++ = static_cast<unsigned char>(std::clamp(std::round(rgb.z * 255.0f), 0.0f, 255.0f));
            *p++ = 255;
        }
    }

    return img;
}

std::vector<QuadVertex> Slicing::BuildQuadVertices(const Volume& volume) const
{
    std::vector<QuadVertex> vertices(12);

    float Ex = volume.GetExtent(0);
    float Ey = volume.GetExtent(1);
    float Ez = volume.GetExtent(2);

    float sx = volume.GetSpacing(0);
    float sy = volume.GetSpacing(1);
    float sz = volume.GetSpacing(2);
    int nx = volume.GetNx();
    int ny = volume.GetNy();
    int nz = volume.GetNz();

    auto makeVertex = [&](const Vector3f& pos, float u, float v) -> QuadVertex {
        return QuadVertex{
            pos,
            u,
            v,
            Vector3f(
                (nx > 0 && sx > 0.0f) ? (pos.x / sx + 0.5f) / nx : 0.5f,
                (ny > 0 && sy > 0.0f) ? (pos.y / sy + 0.5f) / ny : 0.5f,
                (nz > 0 && sz > 0.0f) ? (pos.z / sz + 0.5f) / nz : 0.5f
            )
        };
    };

    // Quad X (YZ plane, x fixed): in-plane axes y (u) and z (v)
    {
        int w = volume.ny;
        int h = volume.nz;
        float u0 = (w > 0) ? 0.5f / w : 0.0f;
        float u1 = (w > 0) ? 1.0f - 0.5f / w : 1.0f;
        float v0 = (h > 0) ? 0.5f / h : 0.0f;
        float v1 = (h > 0) ? 1.0f - 0.5f / h : 1.0f;

        float x = position[0] * Ex;

        // Corners: (x, 0, 0), (x, Ey, 0), (x, Ey, Ez), (x, 0, Ez)
        vertices[0] = makeVertex(Vector3f(x, 0.0f, 0.0f), u0, v0);
        vertices[1] = makeVertex(Vector3f(x, Ey,   0.0f), u1, v0);
        vertices[2] = makeVertex(Vector3f(x, Ey,   Ez),   u1, v1);
        vertices[3] = makeVertex(Vector3f(x, 0.0f, Ez),   u0, v1);
    }

    // Quad Y (ZX plane, y fixed): in-plane axes z (u) and x (v)
    {
        int w = volume.nz;
        int h = volume.nx;
        float u0 = (w > 0) ? 0.5f / w : 0.0f;
        float u1 = (w > 0) ? 1.0f - 0.5f / w : 1.0f;
        float v0 = (h > 0) ? 0.5f / h : 0.0f;
        float v1 = (h > 0) ? 1.0f - 0.5f / h : 1.0f;

        float y = position[1] * Ey;

        // Corners: (0, y, 0), (0, y, Ez), (Ex, y, Ez), (Ex, y, 0)
        vertices[4] = makeVertex(Vector3f(0.0f, y, 0.0f), u0, v0);
        vertices[5] = makeVertex(Vector3f(0.0f, y, Ez),   u1, v0);
        vertices[6] = makeVertex(Vector3f(Ex,   y, Ez),   u1, v1);
        vertices[7] = makeVertex(Vector3f(Ex,   y, 0.0f), u0, v1);
    }

    // Quad Z (XY plane, z fixed): in-plane axes x (u) and y (v)
    {
        int w = volume.nx;
        int h = volume.ny;
        float u0 = (w > 0) ? 0.5f / w : 0.0f;
        float u1 = (w > 0) ? 1.0f - 0.5f / w : 1.0f;
        float v0 = (h > 0) ? 0.5f / h : 0.0f;
        float v1 = (h > 0) ? 1.0f - 0.5f / h : 1.0f;

        float z = position[2] * Ez;

        // Corners: (0, 0, z), (Ex, 0, z), (Ex, Ey, z), (0, Ey, z)
        vertices[8]  = makeVertex(Vector3f(0.0f, 0.0f, z), u0, v0);
        vertices[9]  = makeVertex(Vector3f(Ex,   0.0f, z), u1, v0);
        vertices[10] = makeVertex(Vector3f(Ex,   Ey,   z), u1, v1);
        vertices[11] = makeVertex(Vector3f(0.0f, Ey,   z), u0, v1);
    }

    return vertices;
}

const std::vector<unsigned int>& Slicing::GetQuadIndices()
{
    static const std::vector<unsigned int> indices = {
        0, 1, 2,  0, 2, 3,  // Quad X
        4, 5, 6,  4, 6, 7,  // Quad Y
        8, 9, 10, 8, 10, 11 // Quad Z
    };
    return indices;
}