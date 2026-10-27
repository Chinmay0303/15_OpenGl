#include "isohist.h"
#include "mc_tables.h"

#include <cmath>
#include <algorithm>
#include <iostream>

std::vector<unsigned char> GetRawVolumeBytes(const Volume& volume)
{
    std::size_t total = static_cast<std::size_t>(volume.nx) * volume.ny * volume.nz;
    if (total == 0) return {};

    const unsigned char* p = volume.GetData();
    if (p) {
        return std::vector<unsigned char>(p, p + total);
    }

    std::vector<unsigned char> raw(total);
    for (int z = 0; z < volume.nz; ++z) {
        for (int y = 0; y < volume.ny; ++y) {
            for (int x = 0; x < volume.nx; ++x) {
                raw[x + volume.nx * (y + volume.ny * z)] =
                    static_cast<unsigned char>(std::round(volume.At(x, y, z) * 255.0f));
            }
        }
    }
    return raw;
}

std::vector<long long> BuildIsoHistogram(const Volume& volume)
{
    if (volume.nx < 2 || volume.ny < 2 || volume.nz < 2) {
        return std::vector<long long>(256, 0);
    }

    std::vector<unsigned char> raw = GetRawVolumeBytes(volume);
    std::vector<long long> diff(258, 0);

    const int nx = volume.nx;
    const int ny = volume.ny;
    const int nz = volume.nz;

    for (int z = 0; z < nz - 1; ++z) {
        for (int y = 0; y < ny - 1; ++y) {
            for (int x = 0; x < nx - 1; ++x) {
                std::size_t idx0 = x + nx * (y + ny * z);
                std::size_t idx1 = (x + 1) + nx * (y + ny * z);
                std::size_t idx2 = (x + 1) + nx * ((y + 1) + ny * z);
                std::size_t idx3 = x + nx * ((y + 1) + ny * z);
                std::size_t idx4 = x + nx * (y + ny * (z + 1));
                std::size_t idx5 = (x + 1) + nx * (y + ny * (z + 1));
                std::size_t idx6 = (x + 1) + nx * ((y + 1) + ny * (z + 1));
                std::size_t idx7 = x + nx * ((y + 1) + ny * (z + 1));

                unsigned char c0 = raw[idx0];
                unsigned char c1 = raw[idx1];
                unsigned char c2 = raw[idx2];
                unsigned char c3 = raw[idx3];
                unsigned char c4 = raw[idx4];
                unsigned char c5 = raw[idx5];
                unsigned char c6 = raw[idx6];
                unsigned char c7 = raw[idx7];

                unsigned char m = std::min({c0, c1, c2, c3, c4, c5, c6, c7});
                unsigned char M = std::max({c0, c1, c2, c3, c4, c5, c6, c7});

                if (m < M) {
                    diff[m + 1] += 1;
                    diff[M + 1] -= 1;
                }
            }
        }
    }

    std::vector<long long> cells(256, 0);
    long long running = 0;
    for (int k = 1; k <= 255; ++k) {
        running += diff[k];
        cells[k] = running;
    }
    return cells;
}

long long CountActiveCells(const Volume& volume, int k)
{
    if (k < 1 || k > 255 || volume.nx < 2 || volume.ny < 2 || volume.nz < 2) return 0;

    std::vector<unsigned char> raw = GetRawVolumeBytes(volume);
    long long count = 0;
    const int nx = volume.nx;
    const int ny = volume.ny;
    const int nz = volume.nz;

    for (int z = 0; z < nz - 1; ++z) {
        for (int y = 0; y < ny - 1; ++y) {
            for (int x = 0; x < nx - 1; ++x) {
                std::size_t idx0 = x + nx * (y + ny * z);
                std::size_t idx1 = (x + 1) + nx * (y + ny * z);
                std::size_t idx2 = (x + 1) + nx * ((y + 1) + ny * z);
                std::size_t idx3 = x + nx * ((y + 1) + ny * z);
                std::size_t idx4 = x + nx * (y + ny * (z + 1));
                std::size_t idx5 = (x + 1) + nx * (y + ny * (z + 1));
                std::size_t idx6 = (x + 1) + nx * ((y + 1) + ny * (z + 1));
                std::size_t idx7 = x + nx * ((y + 1) + ny * (z + 1));

                unsigned char c0 = raw[idx0];
                unsigned char c1 = raw[idx1];
                unsigned char c2 = raw[idx2];
                unsigned char c3 = raw[idx3];
                unsigned char c4 = raw[idx4];
                unsigned char c5 = raw[idx5];
                unsigned char c6 = raw[idx6];
                unsigned char c7 = raw[idx7];

                unsigned char m = std::min({c0, c1, c2, c3, c4, c5, c6, c7});
                unsigned char M = std::max({c0, c1, c2, c3, c4, c5, c6, c7});

                if (m < k && k <= M) {
                    count++;
                }
            }
        }
    }
    return count;
}

std::vector<int> FindPeaks(const std::vector<long long>& hist, int radius, int minSeparation, int maxPeaks)
{
    if (hist.size() < 256) return {};

    // 1. Moving average smoothing
    std::vector<double> smoothed(256, 0.0);
    for (int k = 1; k <= 255; ++k) {
        double sum = 0.0;
        int count = 0;
        for (int j = std::max(1, k - radius); j <= std::min(255, k + radius); ++j) {
            sum += static_cast<double>(hist[j]);
            count++;
        }
        smoothed[k] = (count > 0) ? (sum / count) : 0.0;
    }

    // 2. Find local maxima
    struct PeakCandidate {
        int k;
        double height;
    };
    std::vector<PeakCandidate> candidates;

    for (int k = 2; k <= 254; ++k) {
        if (smoothed[k] > smoothed[k - 1] && smoothed[k] > smoothed[k + 1]) {
            candidates.push_back({k, smoothed[k]});
        }
    }

    // 3. Sort by smoothed height descending
    std::sort(candidates.begin(), candidates.end(), [](const PeakCandidate& a, const PeakCandidate& b) {
        return a.height > b.height;
    });

    // 4. Greedily select peaks with separation >= minSeparation
    std::vector<int> peaks;
    for (const auto& cand : candidates) {
        bool ok = true;
        for (int p : peaks) {
            if (std::abs(cand.k - p) < minSeparation) {
                ok = false;
                break;
            }
        }
        if (ok) {
            peaks.push_back(cand.k);
            if (static_cast<int>(peaks.size()) >= maxPeaks) break;
        }
    }

    return peaks;
}

IsoCount CountIso(const Volume& volume, int k)
{
    if (k < 1 || k > 255 || volume.nx < 2 || volume.ny < 2 || volume.nz < 2) return {0, 0};

    std::vector<unsigned char> raw = GetRawVolumeBytes(volume);
    IsoCount res = {0, 0};
    const int nx = volume.nx;
    const int ny = volume.ny;
    const int nz = volume.nz;

    for (int z = 0; z < nz - 1; ++z) {
        for (int y = 0; y < ny - 1; ++y) {
            for (int x = 0; x < nx - 1; ++x) {
                std::size_t idx0 = x + nx * (y + ny * z);
                std::size_t idx1 = (x + 1) + nx * (y + ny * z);
                std::size_t idx2 = (x + 1) + nx * ((y + 1) + ny * z);
                std::size_t idx3 = x + nx * ((y + 1) + ny * z);
                std::size_t idx4 = x + nx * (y + ny * (z + 1));
                std::size_t idx5 = (x + 1) + nx * (y + ny * (z + 1));
                std::size_t idx6 = (x + 1) + nx * ((y + 1) + ny * (z + 1));
                std::size_t idx7 = x + nx * ((y + 1) + ny * (z + 1));

                unsigned char c0 = raw[idx0];
                unsigned char c1 = raw[idx1];
                unsigned char c2 = raw[idx2];
                unsigned char c3 = raw[idx3];
                unsigned char c4 = raw[idx4];
                unsigned char c5 = raw[idx5];
                unsigned char c6 = raw[idx6];
                unsigned char c7 = raw[idx7];

                int cubeindex = 0;
                if (c0 < k) cubeindex |= 1;
                if (c1 < k) cubeindex |= 2;
                if (c2 < k) cubeindex |= 4;
                if (c3 < k) cubeindex |= 8;
                if (c4 < k) cubeindex |= 16;
                if (c5 < k) cubeindex |= 32;
                if (c6 < k) cubeindex |= 64;
                if (c7 < k) cubeindex |= 128;

                if (cubeindex != 0 && cubeindex != 255) {
                    res.cells++;
                    res.triangles += GetTriCount(cubeindex);
                }
            }
        }
    }
    return res;
}

