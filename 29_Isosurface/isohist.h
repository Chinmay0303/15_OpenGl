#ifndef ISOHIST_H
#define ISOHIST_H

#include <vector>
#include "volume.h"

// Task 2.1: Data-Driven Isovalue Selection & Cell Histogram

// Returns a contiguous raw byte copy of the volume voxels
std::vector<unsigned char> GetRawVolumeBytes(const Volume& volume);

// Computes the difference-array histogram: number of cells containing the isosurface for each k in 1..255 (index 0 is 0).
std::vector<long long> BuildIsoHistogram(const Volume& volume);

// Single isovalue active cell counter (O(cells) ground-truth verification)
long long CountActiveCells(const Volume& volume, int k);

// Detects dominant peaks using moving-average smoothing and greedy separation
std::vector<int> FindPeaks(const std::vector<long long>& hist, int radius = 2, int minSeparation = 16, int maxPeaks = 5);

// Task 2.2: CPU Marching Cubes counter (exact ground truth for GPU verification)
struct IsoCount {
    long long cells = 0;
    long long triangles = 0;
};

IsoCount CountIso(const Volume& volume, int k);

#endif // ISOHIST_H

