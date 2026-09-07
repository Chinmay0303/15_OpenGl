#include "slicing.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
constexpr float SeparationStep = 0.05f;
constexpr float MinimumSeparation = 0.0f;
constexpr float Epsilon = 0.000001f;

float Dot(
    const Vector3f& a,
    const Vector3f& b)
{
    return a.x * b.x +
           a.y * b.y +
           a.z * b.z;
}

Vector3f NormalizeVector(const Vector3f& value)
{
    const float length = std::sqrt(Dot(value, value));

    if (length <= Epsilon) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    return Vector3f(
        value.x / length,
        value.y / length,
        value.z / length);
}

bool IsInside(
    float distance,
    bool keepPositive)
{
    constexpr float Epsilon = 0.00001f;

    if (keepPositive) {
        return distance >= -Epsilon;
    }

    return distance <= Epsilon;
}

SliceVertex InterpolateVertexData(
    const SliceVertex& a,
    const SliceVertex& b,
    float t)
{
    SliceVertex result;

    result.position =
        a.position + (b.position - a.position) * t;

    result.colour =
        a.colour + (b.colour - a.colour) * t;

    result.normal =
        NormalizeVector(
            a.normal + (b.normal - a.normal) * t);

    return result;
}

float DistanceSquared(
    const Vector3f& a,
    const Vector3f& b)
{
    const Vector3f difference = a - b;

    return difference.x * difference.x +
           difference.y * difference.y +
           difference.z * difference.z;
}

Vector3f Cross(
    const Vector3f& a,
    const Vector3f& b)
{
    return Vector3f(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

constexpr float SegmentEpsilon = 0.00001f;
constexpr float SegmentEpsilonSquared = SegmentEpsilon * SegmentEpsilon;

}

Slicing::Slicing()
    : mode(SliceMode::Disabled),
      planeCount(1),
      separationDistance(0.05f),
      geometryDirty(false)
{
}

SliceMode Slicing::GetMode() const
{
    return mode;
}

int Slicing::GetPlaneCount() const
{
    return planeCount;
}

float Slicing::GetSeparationDistance() const
{
    return separationDistance;
}

bool Slicing::IsGeometryDirty() const
{
    return geometryDirty;
}

void Slicing::ClearGeometryDirty()
{
    geometryDirty = false;
}

const std::vector<SlicePlane>& Slicing::GetPlanes() const
{
    return planes;
}

Vector3f Slicing::CreateRandomNormal() const
{
    Vector3f normal;

    do {
        normal = Vector3f(
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);

        normal = NormalizeVector(normal);
    } while (Dot(normal, normal) <= Epsilon);

    return normal;
}

bool Slicing::IsAcceptableNormal(
    const Vector3f& normal) const
{
    for (const SlicePlane& plane : planes) {
        const float similarity =
            std::fabs(Dot(normal, plane.normal));

        if (similarity > ParallelThreshold) {
            return false;
        }
    }

    return true;
}

void Slicing::GeneratePlanes()
{
    planes.clear();
    planes.reserve(planeCount);

    while (static_cast<int>(planes.size()) < planeCount) {
        const Vector3f normal = CreateRandomNormal();

        if (IsAcceptableNormal(normal)) {
            planes.push_back({
                normal,
                0.0f
            });
        }
    }

    std::cout
        << "Generated "
        << planeCount
        << " slicing plane(s)\n";

    geometryDirty = true;
}

void Slicing::SetPlaneCount(int count)
{
    if (count < MinPlaneCount) {
        count = MinPlaneCount;
    }

    if (count > MaxPlaneCount) {
        count = MaxPlaneCount;
    }

    planeCount = count;
    GeneratePlanes();

    std::cout
        << "Plane count: "
        << planeCount
        << '\n';
}

void Slicing::ChangeSeparation(float amount)
{
    separationDistance += amount;

    if (separationDistance < MinimumSeparation) {
        separationDistance = MinimumSeparation;
    }

    std::cout
        << "Separation distance: "
        << separationDistance
        << '\n';
}

bool Slicing::HandleKey(unsigned char key)
{
    switch (key) {
    // 'c' is already used for face culling.
    case 'h':
    case 'H':
        mode = mode == SliceMode::CPU
            ? SliceMode::Disabled
            : SliceMode::CPU;
        break;

    case 'g':
    case 'G':
        mode = mode == SliceMode::GPU
            ? SliceMode::Disabled
            : SliceMode::GPU;
        break;

    case 'n':
    case 'N':
        mode = SliceMode::Disabled;
        break;

    case ']':
        SetPlaneCount(planeCount + 1);
        break;

    case '[':
        SetPlaneCount(planeCount - 1);
        break;

    case '>':
    case '.':
        ChangeSeparation(SeparationStep);
        break;

    case '<':
    case ',':
        ChangeSeparation(-SeparationStep);
        break;

    case '\\':
        GeneratePlanes();
        break;

    default:
        return false;
    }

    return true;
}

float Slicing::DistanceToPlane(
    const SlicePlane& plane,
    const Vector3f& point) const
{
    return Dot(plane.normal, point) +
           plane.offset;
}

bool Slicing::NearlyEqual(
    const Vector3f& a,
    const Vector3f& b) const
{
    return DistanceSquared(a, b) <=
           SegmentEpsilonSquared;
}

void Slicing::AddIntersectionPoint(
    std::vector<SliceInputVertex>& points,
    const SliceInputVertex& point) const
{
    for (const SliceInputVertex& existing : points) {
        if (NearlyEqual(
                existing.planePosition,
                point.planePosition)) {
            return;
        }
    }

    points.push_back(point);
}

SliceInputVertex Slicing::InterpolateVertex(
    const SliceInputVertex& a,
    const SliceInputVertex& b,
    float distanceA,
    float distanceB) const
{
    const float denominator =
        distanceA - distanceB;

    float t = 0.0f;

    if (std::fabs(denominator) > Epsilon) {
        t = distanceA / denominator;
    }

    t = std::fmax(0.0f, std::fmin(1.0f, t));

    SliceInputVertex result;

    result.vertex =
        InterpolateVertexData(
            a.vertex,
            b.vertex,
            t);

    result.planePosition =
        a.planePosition +
        (b.planePosition - a.planePosition) * t;

    return result;
}

std::vector<SliceInputVertex> Slicing::ClipPolygon(
    const std::vector<SliceInputVertex>& polygon,
    const SlicePlane& plane,
    bool keepPositive) const
{
    std::vector<SliceInputVertex> result;

    if (polygon.empty()) {
        return result;
    }

    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const SliceInputVertex& current =
            polygon[i];

        const SliceInputVertex& next =
            polygon[(i + 1) % polygon.size()];

        const float currentDistance =
            DistanceToPlane(
                plane,
                current.planePosition);

        const float nextDistance =
            DistanceToPlane(
                plane,
                next.planePosition);

        const bool currentInside =
            IsInside(
                currentDistance,
                keepPositive);

        const bool nextInside =
            IsInside(
                nextDistance,
                keepPositive);

        if (currentInside && nextInside) {
            result.push_back(next);
        }
        else if (currentInside && !nextInside) {
            result.push_back(
                InterpolateVertex(
                    current,
                    next,
                    currentDistance,
                    nextDistance));
        }
        else if (!currentInside && nextInside) {
            result.push_back(
                InterpolateVertex(
                    current,
                    next,
                    currentDistance,
                    nextDistance));

            result.push_back(next);
        }
    }

    return result;
}

void Slicing::SetPlanePoint(
    std::size_t planeIndex,
    const Vector3f& point)
{
    if (planeIndex >= planes.size()) {
        return;
    }

    SlicePlane& plane = planes[planeIndex];

    // offset = -dot(normal, pointOnPlane)
    plane.offset = -Dot(plane.normal, point);

    geometryDirty = true;
}

PlaneSide Slicing::ClassifyPoint(
    const SlicePlane& plane,
    const Vector3f& point) const
{
    const float distance =
        DistanceToPlane(plane, point);

    if (distance > PlaneEpsilon) {
        return PlaneSide::Positive;
    }

    if (distance < -PlaneEpsilon) {
        return PlaneSide::Negative;
    }

    return PlaneSide::OnPlane;
}

TriangleGroups Slicing::ClassifyTriangles(
    const Vector3f* vertices,
    const unsigned int* indices,
    int indexCount) const
{
    TriangleGroups groups;

    if (planes.empty()) {
        return groups;
    }

    const SlicePlane& plane = planes.front();

    for (int i = 0; i < indexCount; i += 3) {
        const unsigned int index0 = indices[i];
        const unsigned int index1 = indices[i + 1];
        const unsigned int index2 = indices[i + 2];

        const float d0 =
            DistanceToPlane(plane, vertices[index0]);

        const float d1 =
            DistanceToPlane(plane, vertices[index1]);

        const float d2 =
            DistanceToPlane(plane, vertices[index2]);

        const bool hasPositive =
            d0 > PlaneEpsilon ||
            d1 > PlaneEpsilon ||
            d2 > PlaneEpsilon;

        const bool hasNegative =
            d0 < -PlaneEpsilon ||
            d1 < -PlaneEpsilon ||
            d2 < -PlaneEpsilon;

        if (hasPositive && hasNegative) {
            groups.intersectingIndices.push_back(index0);
            groups.intersectingIndices.push_back(index1);
            groups.intersectingIndices.push_back(index2);
        }
        else if (hasPositive) {
            groups.positiveIndices.push_back(index0);
            groups.positiveIndices.push_back(index1);
            groups.positiveIndices.push_back(index2);
        }
        else {
            groups.negativeIndices.push_back(index0);
            groups.negativeIndices.push_back(index1);
            groups.negativeIndices.push_back(index2);
        }
    }

    return groups;
}

void Slicing::AddPolygon(
    SliceMesh& mesh,
    const std::vector<SliceInputVertex>& polygon) const
{
    if (polygon.size() < 3) {
        return;
    }

    const unsigned int baseIndex =
        static_cast<unsigned int>(
            mesh.vertices.size());

    for (const SliceInputVertex& vertex : polygon) {
        mesh.vertices.push_back(vertex.vertex);
    }

    for (std::size_t i = 1;
         i + 1 < polygon.size();
         ++i) {

        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(
            baseIndex + static_cast<unsigned int>(i));
        mesh.indices.push_back(
            baseIndex + static_cast<unsigned int>(i + 1));
    }
}

void Slicing::AddCapForLoop(
    SliceMesh& mesh,
    const std::vector<SliceInputVertex>& loop,
    const Vector3f& capNormal) const
{
if (loop.size() < 3) {
        return;
    }

    Vector3f geometricNormal(0.0f, 0.0f, 0.0f);

    for (std::size_t i = 1;
         i + 1 < loop.size();
         ++i) {
        const Vector3f edge1 =
            loop[i].vertex.position -
            loop[0].vertex.position;

        const Vector3f edge2 =
            loop[i + 1].vertex.position -
            loop[0].vertex.position;

        geometricNormal += Cross(
            edge1,
            edge2);
    }

    geometricNormal =
        NormalizeVector(geometricNormal);

    // Preserve the requested cap orientation.
    Vector3f finalNormal = geometricNormal;

    if (Dot(
            finalNormal,
            capNormal) < 0.0f) {
        finalNormal = finalNormal * -1.0f;
    }

    Vector3f center(0.0f, 0.0f, 0.0f);

    for (const SliceInputVertex& point : loop) {
        center += point.vertex.position;
    }

    center = center *
        (1.0f / static_cast<float>(loop.size()));

    SliceVertex centerVertex;
    centerVertex.position = center;
    centerVertex.colour = Vector3f(1.0f, 1.0f, 1.0f);
    centerVertex.normal = finalNormal;

    const unsigned int centerIndex =
        static_cast<unsigned int>(mesh.vertices.size());

    mesh.vertices.push_back(centerVertex);

    std::vector<unsigned int> loopIndices;
    loopIndices.reserve(loop.size());

    for (const SliceInputVertex& point : loop) {
        SliceVertex capVertex = point.vertex;
        capVertex.normal = capNormal;

        loopIndices.push_back(
            static_cast<unsigned int>(mesh.vertices.size()));

        mesh.vertices.push_back(capVertex);
    }

    for (std::size_t i = 0; i < loopIndices.size(); ++i) {
        const std::size_t next =
            (i + 1) % loopIndices.size();

        mesh.indices.push_back(centerIndex);
        mesh.indices.push_back(loopIndices[i]);
        mesh.indices.push_back(loopIndices[next]);
    }
}

ClippedMeshes Slicing::ClipMesh(
    const std::vector<SliceInputVertex>& vertices,
    const std::vector<unsigned int>& indices) const
{
    ClippedMeshes result;

    if (planes.empty()) {
        return result;
    }

    const SlicePlane& plane = planes.front();

    // Stores one cutting-plane segment for each intersected triangle.
    std::vector<SliceSegment> segments;

    // ------------------------------------------------------------
    // 1. Clip every triangle and collect its intersection segment.
    // ------------------------------------------------------------
    for (std::size_t i = 0;
         i + 2 < indices.size();
         i += 3) {

        const unsigned int index0 = indices[i];
        const unsigned int index1 = indices[i + 1];
        const unsigned int index2 = indices[i + 2];

        if (index0 >= vertices.size() ||
            index1 >= vertices.size() ||
            index2 >= vertices.size()) {
            continue;
        }

        const std::vector<SliceInputVertex> triangle{
            vertices[index0],
            vertices[index1],
            vertices[index2]
        };

        std::vector<SliceInputVertex> intersectionPoints;

        const SliceInputVertex triangleVertices[3] = {
            vertices[index0],
            vertices[index1],
            vertices[index2]
        };

        // Check the three triangle edges.
        for (int edge = 0; edge < 3; ++edge) {
            const SliceInputVertex& a =
                triangleVertices[edge];

            const SliceInputVertex& b =
                triangleVertices[(edge + 1) % 3];

            const float distanceA =
                DistanceToPlane(
                    plane,
                    a.planePosition);

            const float distanceB =
                DistanceToPlane(
                    plane,
                    b.planePosition);

            const bool crosses =
                (distanceA > PlaneEpsilon &&
                 distanceB < -PlaneEpsilon) ||
                (distanceA < -PlaneEpsilon &&
                 distanceB > PlaneEpsilon);

            if (crosses) {
                const SliceInputVertex intersection =
                    InterpolateVertex(
                        a,
                        b,
                        distanceA,
                        distanceB);

                AddIntersectionPoint(
                    intersectionPoints,
                    intersection);
            }
        }

        // A triangle crossing the plane produces one segment.
        if (intersectionPoints.size() == 2) {
            segments.push_back({
                intersectionPoints[0],
                intersectionPoints[1]
            });
        }

        // Clip the triangle to both sides.
        const std::vector<SliceInputVertex>
            positivePolygon =
                ClipPolygon(
                    triangle,
                    plane,
                    true);

        const std::vector<SliceInputVertex>
            negativePolygon =
                ClipPolygon(
                    triangle,
                    plane,
                    false);

        AddPolygon(
            result.positive,
            positivePolygon);

        AddPolygon(
            result.negative,
            negativePolygon);
    }

    // ------------------------------------------------------------
    // 2. Connect intersection segments into boundary loops.
    // ------------------------------------------------------------
    std::vector<bool> used(
        segments.size(),
        false);

    for (std::size_t start = 0;
         start < segments.size();
         ++start) {

        if (used[start]) {
            continue;
        }

        std::vector<SliceInputVertex> loop;

        loop.push_back(segments[start].start);
        loop.push_back(segments[start].end);

        used[start] = true;

        Vector3f currentPoint =
            segments[start].end.planePosition;

        bool closed = false;

        while (!closed) {
            bool foundNextSegment = false;

            for (std::size_t i = 0;
                 i < segments.size();
                 ++i) {

                if (used[i]) {
                    continue;
                }

                const bool matchesStart =
                    NearlyEqual(
                        currentPoint,
                        segments[i].start.planePosition);

                const bool matchesEnd =
                    NearlyEqual(
                        currentPoint,
                        segments[i].end.planePosition);

                if (!matchesStart && !matchesEnd) {
                    continue;
                }

                const SliceInputVertex& nextPoint =
                    matchesStart
                        ? segments[i].end
                        : segments[i].start;

                loop.push_back(nextPoint);

                currentPoint =
                    nextPoint.planePosition;

                used[i] = true;
                foundNextSegment = true;
                break;
            }

            if (NearlyEqual(
                    currentPoint,
                    loop.front().planePosition)) {
                closed = true;
            }
            else if (!foundNextSegment) {
                // The loop is open or numerically invalid.
                break;
            }
        }

        // The final point duplicates the first point.
        if (closed && loop.size() >= 4) {
            loop.pop_back();

            // Positive half: cap normal points along +plane.normal.
            AddCapForLoop(
                result.positive,
                loop,
                plane.normal);

            // Negative half: reverse winding and use the opposite normal.
            std::vector<SliceInputVertex>
                reversedLoop(
                    loop.rbegin(),
                    loop.rend());

            AddCapForLoop(
                result.negative,
                reversedLoop,
                plane.normal * -1.0f);
        }
    }

    return result;
}