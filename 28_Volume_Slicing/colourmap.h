#ifndef COLOURMAP_H
#define COLOURMAP_H

#include "ogldev_math_3d.h"
#include <vector>

enum class MapType {
    DIVERGENT_1 = 0,
    GRAYSCALE = 1,
    VIRIDIS = 2
};

// Transfer function / colormap lookup table (256-entry precomputed LUT)
class Colourmap {
public:
    static constexpr int ColormapSize = 256;

    Colourmap(MapType type = MapType::DIVERGENT_1);

    // Map normalized scalar s in [0, 1] to RGB [0, 1]
    Vector3f Lookup(float s) const;

    void SetType(MapType type);
    MapType GetType() const { return currentType_; }
    const char* GetTypeName() const;
    void CycleType();

    unsigned int CreateGLTexture1D() const;
    void UpdateGLTexture1D(unsigned int textureId) const;

private:
    void Rebuild();
    MapType currentType_ = MapType::DIVERGENT_1;
    Vector3f table_[ColormapSize];
};

#endif // COLOURMAP_H
