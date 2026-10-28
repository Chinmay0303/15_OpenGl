#ifndef SLICING_H
#define SLICING_H

#include <vector>
#include "ogldev_math_3d.h"
#include "volume.h"
#include "colourmap.h"

// Mapping:
// X = YZ plane (x fixed, Sagittal: divides Left / Right)
// Y = ZX plane (y fixed, Axial: divides Top / Bottom)
// Z = XY plane (z fixed, Coronal: divides Front / Back)
enum class SliceAxis {
    X = 0,
    Y = 1,
    Z = 2
};

struct SliceImage {
    int width;
    int height;
    std::vector<unsigned char> pixels; // RGBA8
};

struct QuadVertex {
    Vector3f position;
    float u;
    float v;
    Vector3f texCoord3D;
};

class Slicing {
public:
    Slicing();

    // Key handling: x/X, y/Y, z/Z step slices; 7/8/9 toggle visibility
    bool HandleKey(unsigned char key, const Volume& volume);

    bool IsDirty(SliceAxis axis) const;
    void ClearDirty(SliceAxis axis);
    void MarkAllDirty() { dirty[0] = dirty[1] = dirty[2] = true; }

    float GetPosition(SliceAxis axis) const;
    void SetPosition(SliceAxis axis, float t);

    bool IsVisible(SliceAxis axis) const;
    void SetVisible(SliceAxis axis, bool v);

    int GetStepVoxels() const { return stepVoxels; }
    void SetStepVoxels(int s) { stepVoxels = (s > 0) ? s : 1; }

    int GetSliceIndex(SliceAxis axis, const Volume& volume) const;
    void SetSliceIndex(SliceAxis axis, int index, const Volume& volume);
    void InitPositions(const Volume& volume);

    SliceImage BuildSliceImage(const Volume& volume, const Colourmap& colormap, SliceAxis axis) const;
    std::vector<QuadVertex> BuildQuadVertices(const Volume& volume) const;

    static const std::vector<unsigned int>& GetQuadIndices();

private:
    int stepVoxels = 1;
    float position[3]; // normalized t in [0, 1], default 0.5
    bool visible[3];   // default true
    bool dirty[3];     // default true
};

#endif // SLICING_H