#ifndef SLICING_H
#define SLICING_H

#include <vector>
#include <cstddef>
#include "ogldev_math_3d.h"

enum class SliceMode {
    Disabled,
    CPU,
    GPU
};

enum class PlaneSide {
    Negative,
    OnPlane,
    Positive
};

enum class TriangleSide {
    Positive,
    Negative,
    Intersecting
};

struct TriangleGroups {
    std::vector<unsigned int> positiveIndices;
    std::vector<unsigned int> negativeIndices;
    std::vector<unsigned int> intersectingIndices;
};

struct SlicePlane {
    Vector3f normal;
    float offset; // n · p + offset = 0
};

struct SliceVertex {
    Vector3f position;
    Vector3f colour;
    Vector3f normal;
};

struct SliceInputVertex {
    SliceVertex vertex;
    Vector3f planePosition;
};

struct SliceSegment {
    SliceInputVertex start;
    SliceInputVertex end;
};

struct SliceMesh {
    std::vector<SliceVertex> vertices;
    std::vector<unsigned int> indices;
};

struct ClippedMeshes {
    SliceMesh positive;
    SliceMesh negative;
    SliceMesh caps;
    std::vector<SliceMesh> positiveRegions;
};

class Slicing {
public:
    Slicing();

    bool HandleKey(unsigned char key);

    void GeneratePlanes();

    float DistanceToPlane(
        const SlicePlane& plane,
        const Vector3f& point) const;

    PlaneSide ClassifyPoint(
        const SlicePlane& plane,
        const Vector3f& point) const;

    TriangleGroups ClassifyTriangles(
        const Vector3f* vertices,
        const unsigned int* indices,
        int indexCount) const;

    void SetPlanePoint(
        std::size_t planeIndex,
        const Vector3f& point);

    SliceMode GetMode() const;
    int GetPlaneCount() const;
    float GetSeparationDistance() const;
    
    bool IsGeometryDirty() const;
    void ClearGeometryDirty();

    ClippedMeshes ClipMesh(
    const std::vector<SliceInputVertex>& vertices,
    const std::vector<unsigned int>& indices) const;

    const std::vector<SlicePlane>& GetPlanes() const;

private:
    void SetPlaneCount(int count);
    void ChangeSeparation(float amount);

    Vector3f CreateRandomNormal() const;
    float CreateRandomPlaneOffset(
        const Vector3f& normal) const;
    bool IsAcceptableNormal(
        const Vector3f& normal) const;

    ClippedMeshes ClipMultiplePlanes(
    const std::vector<SliceInputVertex>& vertices,
    const std::vector<unsigned int>& indices) const;

    std::vector<SliceInputVertex> ClipPolygon(
        const std::vector<SliceInputVertex>& polygon,
        const SlicePlane& plane,
        bool keepPositive) const;

    SliceInputVertex InterpolateVertex(
        const SliceInputVertex& a,
        const SliceInputVertex& b,
        float distanceA,
        float distanceB) const;

    void AddPolygon(
        SliceMesh& mesh,
        const std::vector<SliceInputVertex>& polygon) const;

    void AddCapForLoop(
        SliceMesh& mesh,
        const std::vector<SliceInputVertex>& loop,
        const Vector3f& capNormal) const;

    bool NearlyEqual(
        const Vector3f& a,
        const Vector3f& b) const;

    void AddIntersectionPoint(
        std::vector<SliceInputVertex>& points,
        const SliceInputVertex& point) const;

private:
    static constexpr int MinPlaneCount = 1;
    static constexpr int MaxPlaneCount = 10;

    static constexpr float ParallelThreshold = 0.95f;
    static constexpr float PlaneEpsilon = 0.00001f;

    SliceMode mode;
    int planeCount;
    float separationDistance;

    std::vector<SlicePlane> planes;

    bool geometryDirty;
};

#endif