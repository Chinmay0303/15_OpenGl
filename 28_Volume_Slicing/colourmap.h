#ifndef COLOURMAP_H
#define COLOURMAP_H

#include "ogldev_math_3d.h"
#include <vector>

// Divergent 1 colormap: Lab-space diverging transfer function (47 control points),
// 256-entry precomputed LUT. Lightness profile: dark purple (0.0) -> light cream (0.5) -> dark maroon (1.0).
class Colourmap {
public:
    static constexpr int ColormapSize = 256;

    Colourmap();

    // Map normalized scalar s in [0, 1] to RGB [0, 1]
    Vector3f Lookup(float s) const;

    unsigned int CreateGLTexture1D() const;

private:
    void Rebuild();
    Vector3f table_[ColormapSize];
};

using Colormap = Colourmap;

#endif // COLOURMAP_H
