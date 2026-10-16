#ifndef VOLUME_H
#define VOLUME_H

#include <vector>
#include <string>
#include <cstdint>

// Stores uint8 voxels [0, 255]. At() returns normalized float in [0, 1].
class Volume {
public:
    int nx = 0, ny = 0, nz = 0;          // voxel dimensions
    float sx = 1.0f, sy = 1.0f, sz = 1.0f; // spacing (physical size per voxel)

    // Load raw uint8 or uint16 file. Auto-detects 8-bit vs 16-bit based on file size.
    bool LoadRaw(const std::string& path, int nx, int ny, int nz,
                 float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);

    bool LoadUint8(const std::string& path, int nx, int ny, int nz,
                   float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) {
        return LoadRaw(path, nx, ny, nz, sx, sy, sz);
    }

    // Synthetic datasets (dims 64 x 48 x 32 by default):
    // "ramp", "sphere", "corner"
    void CreateSynthetic(const std::string& name, int nx = 64, int ny = 48, int nz = 32);

    // Direct voxel access (clamped indices). Returns normalized float in [0, 1].
    float At(int i, int j, int k) const;

    // Create OpenGL 3D texture (GL_R8, GL_LINEAR, GL_CLAMP_TO_EDGE)
    unsigned int CreateGLTexture3D() const;

    // Trilinear sample at continuous voxel coordinates [0, n-1]. Returns float in [0, 1].
    float Sample(float x, float y, float z) const;

    int GetNx() const { return nx; }
    int GetNy() const { return ny; }
    int GetNz() const { return nz; }

    float GetSpacing(int axis) const {
        if (axis == 0) return sx;
        if (axis == 1) return sy;
        return sz;
    }

    float GetExtent(int axis) const {
        if (axis == 0) return (nx > 1) ? (nx - 1) * sx : 0.0f;
        if (axis == 1) return (ny > 1) ? (ny - 1) * sy : 0.0f;
        return (nz > 1) ? (nz - 1) * sz : 0.0f;
    }

private:
    std::vector<unsigned char> data_; // raw uint8 voxels
};

#endif // VOLUME_H
