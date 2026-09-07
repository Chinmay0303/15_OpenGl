#include "slicing.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>

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

Vector3f AnyPerpendicular(const Vector3f& normal)
{
    const Vector3f reference =
        std::fabs(normal.x) < 0.9f
            ? Vector3f(1.0f, 0.0f, 0.0f)
            : Vector3f(0.0f, 1.0f, 0.0f);

    return NormalizeVector(Cross(normal, reference));
}

float Cross2D(
    const Vector2f& a,
    const Vector2f& b,
    const Vector2f& c)
{
    return (b.x - a.x) * (c.y - a.y) -
           (b.y - a.y) * (c.x - a.x);
}

float PolygonArea(const std::vector<Vector2f>& polygon)
{
    float area = 0.0f;

    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vector2f& current = polygon[i];
        const Vector2f& next = polygon[(i + 1) % polygon.size()];
        area += current.x * next.y - next.x * current.y;
    }

    return area * 0.5f;
}

bool PointInsideTriangle(
    const Vector2f& point,
    const Vector2f& a,
    const Vector2f& b,
    const Vector2f& c,
    float winding)
{
    constexpr float Epsilon = 0.000001f;

    return winding * Cross2D(a, b, point) >= -Epsilon &&
           winding * Cross2D(b, c, point) >= -Epsilon &&
           winding * Cross2D(c, a, point) >= -Epsilon;
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
        Vector3f normal1 = Vector3f(
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            0.0f);

        Vector3f normal2 = Vector3f(
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f,
            0.0f);
        
        normal = normal1 - normal2;   
        normal = NormalizeVector(normal);
    } while (Dot(normal, normal) <= Epsilon);

    return normal;
}

float Slicing::CreateRandomPlaneOffset(
    const Vector3f& normal) const
{
    constexpr float PlanePositionRange = 0.20f;

    const Vector3f point(
        (2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f) *
            PlanePositionRange,
        (2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f) *
            PlanePositionRange,
        0.0f);

    return -Dot(normal, point);
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
                CreateRandomPlaneOffset(normal)
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
    if (planeCount > 1) {
        // separationDistance = 0.05f;
        separationDistance = 0.05f;

        std::cout
            << "Separation disabled for multiple planes\n";

        return;
    }

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
        if (mode == SliceMode::CPU) {
            mode = SliceMode::Disabled;
        }
        else {
            mode = SliceMode::CPU;
            geometryDirty = true;
        }
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

    for (std::size_t i = 0; i < loop.size(); ++i) {
        const Vector3f& current =
            loop[i].vertex.position;

        const Vector3f& next =
            loop[(i + 1) % loop.size()].vertex.position;

        geometricNormal += Cross(
            current,
            next);
    }

    Vector3f finalNormal =
        NormalizeVector(geometricNormal);
    if (Dot(finalNormal, capNormal) < 0.0f) {
        finalNormal = finalNormal * -1.0f;
    }

    if (Dot(finalNormal, finalNormal) <= Epsilon) {
        return;
    }

    const Vector3f tangent = AnyPerpendicular(finalNormal);
    const Vector3f bitangent = Cross(finalNormal, tangent);

    std::vector<Vector2f> projected;
    projected.reserve(loop.size());

    for (const SliceInputVertex& point : loop) {
        const Vector3f& position = point.vertex.position;

        projected.emplace_back(
            Vector2f(
                Dot(position, tangent),
                Dot(position, bitangent)));
    }

    const float signedArea = PolygonArea(projected);

    if (std::fabs(signedArea) <= Epsilon) {
        return;
    }

    const float winding = signedArea > 0.0f ? 1.0f : -1.0f;

    std::vector<std::size_t> remaining;
    remaining.reserve(loop.size());

    for (std::size_t i = 0; i < loop.size(); ++i) {
        remaining.push_back(i);
    }

    while (remaining.size() > 3) {
        bool earFound = false;

        for (std::size_t i = 0; i < remaining.size(); ++i) {
            const std::size_t previous =
                remaining[(i + remaining.size() - 1) % remaining.size()];
            const std::size_t current = remaining[i];
            const std::size_t next =
                remaining[(i + 1) % remaining.size()];

            if (winding * Cross2D(
                    projected[previous],
                    projected[current],
                    projected[next]) <= Epsilon) {
                continue;
            }

            bool containsPoint = false;

            for (const std::size_t candidate : remaining) {
                if (candidate == previous ||
                    candidate == current ||
                    candidate == next) {
                    continue;
                }

                if (PointInsideTriangle(
                        projected[candidate],
                        projected[previous],
                        projected[current],
                        projected[next],
                        winding)) {
                    containsPoint = true;
                    break;
                }
            }

            if (containsPoint) {
                continue;
            }

            const unsigned int baseIndex =
                static_cast<unsigned int>(mesh.vertices.size());

            SliceVertex first = loop[previous].vertex;
            SliceVertex second = loop[current].vertex;
            SliceVertex third = loop[next].vertex;

            first.normal = finalNormal;
            second.normal = finalNormal;
            third.normal = finalNormal;

            mesh.vertices.push_back(first);
            mesh.vertices.push_back(second);
            mesh.vertices.push_back(third);

            mesh.indices.push_back(baseIndex);
            mesh.indices.push_back(baseIndex + 1);
            mesh.indices.push_back(baseIndex + 2);

            remaining.erase(remaining.begin() + i);
            earFound = true;
            break;
        }

        if (!earFound) {
            return;
        }
    }

    if (remaining.size() == 3) {
        const unsigned int baseIndex =
            static_cast<unsigned int>(mesh.vertices.size());

        SliceVertex first = loop[remaining[0]].vertex;
        SliceVertex second = loop[remaining[1]].vertex;
        SliceVertex third = loop[remaining[2]].vertex;

        first.normal = finalNormal;
        second.normal = finalNormal;
        third.normal = finalNormal;

        mesh.vertices.push_back(first);
        mesh.vertices.push_back(second);
        mesh.vertices.push_back(third);

        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 1);
        mesh.indices.push_back(baseIndex + 2);
    }
}

ClippedMeshes Slicing::ClipMultiplePlanes(
    const std::vector<SliceInputVertex>& vertices,
    const std::vector<unsigned int>& indices) const
{
    ClippedMeshes result;

    if (planes.empty()) {
        return result;
    }

    const std::size_t planeCountValue =
        planes.size();

    const std::size_t regionCount =
        std::size_t{1} << planeCountValue;

    // One independent mesh for each sign-mask region.
    std::vector<SliceMesh> regionMeshes(regionCount);

    // One segment list for each plane and region.
    //
    // A segment stored under [planeIndex, mask] belongs to the
    // boundary of that region on that plane.
    std::vector<std::vector<SliceSegment>>
        boundarySegments(
            planeCountValue * regionCount);

    auto segmentList = [&](std::size_t planeIndex,
                           std::size_t mask)
        -> std::vector<SliceSegment>&
    {
        return boundarySegments[
            planeIndex * regionCount + mask];
    };

    auto addUniquePoint =
        [this](
            std::vector<SliceInputVertex>& points,
            const SliceInputVertex& point)
    {
        for (const SliceInputVertex& existing : points) {
            if (NearlyEqual(
                    existing.planePosition,
                    point.planePosition)) {
                return;
            }
        }

        points.push_back(point);
    };

    // ------------------------------------------------------------
    // Clip every triangle into every possible sign-mask region.
    // ------------------------------------------------------------
    for (std::size_t triangleIndex = 0;
         triangleIndex + 2 < indices.size();
         triangleIndex += 3) {

        const unsigned int index0 =
            indices[triangleIndex];

        const unsigned int index1 =
            indices[triangleIndex + 1];

        const unsigned int index2 =
            indices[triangleIndex + 2];

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

        std::function<void(
            std::size_t,
            std::size_t,
            const std::vector<SliceInputVertex>&)> visitRegion;

        visitRegion = [&, this](
            std::size_t planeIndex,
            std::size_t mask,
            const std::vector<SliceInputVertex>& polygon)
        {
            if (polygon.size() < 3) {
                return;
            }

            if (planeIndex == planeCountValue) {
                AddPolygon(
                    regionMeshes[mask],
                    polygon);

                for (std::size_t boundaryPlane = 0;
                     boundaryPlane < planeCountValue;
                     ++boundaryPlane) {
                    std::vector<SliceInputVertex>
                        boundaryPoints;

                    for (const SliceInputVertex& point : polygon) {
                        const float distance =
                            DistanceToPlane(
                                planes[boundaryPlane],
                                point.planePosition);

                        if (std::fabs(distance) <= PlaneEpsilon) {
                            addUniquePoint(
                                boundaryPoints,
                                point);
                        }
                    }

                    if (boundaryPoints.size() == 2) {
                        segmentList(
                            boundaryPlane,
                            mask).push_back({
                                boundaryPoints[0],
                                boundaryPoints[1]
                            });
                    }
                }

                return;
            }

            const std::size_t planeBit =
                std::size_t{1} << planeIndex;

            const std::vector<SliceInputVertex> positivePolygon =
                ClipPolygon(
                    polygon,
                    planes[planeIndex],
                    true);

            visitRegion(
                planeIndex + 1,
                mask | planeBit,
                positivePolygon);

            const std::vector<SliceInputVertex> negativePolygon =
                ClipPolygon(
                    polygon,
                    planes[planeIndex],
                    false);

            visitRegion(
                planeIndex + 1,
                mask,
                negativePolygon);
        };

        visitRegion(0, 0, triangle);
    }

    std::size_t capCount = 0;

    // ------------------------------------------------------------
    // Connect segments into loops and cap both adjacent regions.
    // ------------------------------------------------------------
    for (std::size_t planeIndex = 0;
         planeIndex < planeCountValue;
         ++planeIndex) {

        const SlicePlane& plane =
            planes[planeIndex];

        // Process only masks whose plane bit is positive.
        // The adjacent negative region is mask with that bit cleared.
        for (std::size_t mask = 0;
             mask < regionCount;
             ++mask) {

            const std::size_t planeBit =
                std::size_t{1} << planeIndex;

            if ((mask & planeBit) == 0) {
                continue;
            }

            std::vector<SliceSegment>& segments =
                segmentList(planeIndex, mask);

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
                    bool foundNext = false;

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
                        foundNext = true;
                        break;
                    }

                    if (NearlyEqual(
                            currentPoint,
                            loop.front().planePosition)) {
                        closed = true;
                    }
                    else if (!foundNext) {
                        break;
                    }
                }

                if (!closed || loop.size() < 4) {
                    continue;
                }

                // Remove duplicate closing point.
                loop.pop_back();

                const std::size_t negativeMask =
                    mask ^ planeBit;

                // Positive side of this plane.
                AddCapForLoop(
                    regionMeshes[mask],
                    loop,
                    plane.normal);

                // Negative side of this plane.
                std::vector<SliceInputVertex> reversedLoop(
                    loop.rbegin(),
                    loop.rend());

                AddCapForLoop(
                    regionMeshes[negativeMask],
                    reversedLoop,
                    plane.normal * -1.0f);
                
                ++capCount;
            }
        }
    }

    std::size_t nonEmptyRegionCount = 0;

    for (const SliceMesh& regionMesh : regionMeshes) {
        if (!regionMesh.indices.empty()) {
            ++nonEmptyRegionCount;
        }
    }

    result.positiveRegions = std::move(regionMeshes);

    std::cout
        << "[Multi-plane CPU] planes="
        << planeCountValue
        << ", regions="
        << nonEmptyRegionCount
        << "/"
        << regionCount
        << ", caps="
        << capCount
        << ", triangles="
        << std::accumulate(
            result.positiveRegions.begin(),
            result.positiveRegions.end(),
            std::size_t{0},
            [](std::size_t total, const SliceMesh& mesh) {
                return total + mesh.indices.size() / 3;
            })
        << '\n';

    return result;
}

ClippedMeshes Slicing::ClipMesh(
    const std::vector<SliceInputVertex>& vertices,
    const std::vector<unsigned int>& indices) const
{
    ClippedMeshes result;

    if (planes.empty()) {
        return result;
    }

    if (planes.size() > 1) {
        return ClipMultiplePlanes(
            vertices,
            indices);
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

        // Check the three triangle edges, including vertices already on the plane.
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

            if (std::fabs(distanceA) <= PlaneEpsilon) {
                AddIntersectionPoint(
                    intersectionPoints,
                    a);
            }

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