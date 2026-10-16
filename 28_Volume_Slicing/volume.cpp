#include "volume.h"

#include <GL/glew.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstdio>



bool Volume::LoadRaw(const std::string& path,
                      int nx_, int ny_, int nz_,
                      float sx_, float sy_, float sz_)
{
    const std::size_t numVoxels = static_cast<std::size_t>(nx_) * ny_ * nz_;
    const std::size_t expectedU8 = numVoxels;
    const std::size_t expectedU16 = numVoxels * 2;

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "[Volume] cannot open: " << path << '\n';
        return false;
    }

    f.seekg(0, std::ios::end);
    std::size_t actualSize = static_cast<std::size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    bool isUint16 = false;
    if (actualSize == expectedU8) {
        isUint16 = false;
    } else if (actualSize == expectedU16) {
        isUint16 = true;
    } else {
        std::cerr << "[Volume] size mismatch: expected " << expectedU8
                  << " (uint8) or " << expectedU16 << " (uint16) bytes, got " << actualSize << '\n';
        return false;
    }

    // Assign volume properties after validation succeeds
    nx = nx_; ny = ny_; nz = nz_;
    sx = sx_; sy = sy_; sz = sz_;
    data_.resize(numVoxels);

    if (isUint16) {
        std::vector<uint16_t> temp(numVoxels);
        f.read(reinterpret_cast<char*>(temp.data()), static_cast<std::streamsize>(expectedU16));

        uint16_t minVal = temp[0], maxVal = temp[0];
        for (std::size_t i = 1; i < numVoxels; ++i) {
            if (temp[i] < minVal) minVal = temp[i];
            if (temp[i] > maxVal) maxVal = temp[i];
        }

        float invRange = (maxVal > minVal) ? 255.0f / static_cast<float>(maxVal - minVal) : 0.0f;
        for (std::size_t i = 0; i < numVoxels; ++i) {
            data_[i] = static_cast<unsigned char>((temp[i] - minVal) * invRange);
        }

        std::cout << "[Volume] loaded uint16 " << path << " (" << nx << "x" << ny << "x" << nz
                  << ") raw min=" << minVal << " max=" << maxVal << " -> mapped to [0, 255]\n";
    } else {
        f.read(reinterpret_cast<char*>(data_.data()), static_cast<std::streamsize>(expectedU8));

        unsigned char minVal = data_[0], maxVal = data_[0];
        for (std::size_t i = 1; i < numVoxels; ++i) {
            if (data_[i] < minVal) minVal = data_[i];
            if (data_[i] > maxVal) maxVal = data_[i];
        }

        std::cout << "[Volume] loaded uint8 " << path << " (" << nx << "x" << ny << "x" << nz
                  << ") raw min=" << static_cast<int>(minVal)
                  << " max=" << static_cast<int>(maxVal) << '\n';

        if (minVal < maxVal && (minVal > 0 || maxVal < 255)) {
            float invRange = 255.0f / static_cast<float>(maxVal - minVal);
            for (std::size_t i = 0; i < numVoxels; ++i) {
                data_[i] = static_cast<unsigned char>((data_[i] - minVal) * invRange);
            }
            std::cout << "[Volume] Normalized dynamic range [" << static_cast<int>(minVal)
                      << ", " << static_cast<int>(maxVal) << "] -> [0, 255]\n";
        }
    }

    return true;
}

void Volume::CreateSynthetic(const std::string& name, int nx_, int ny_, int nz_)
{
    nx = nx_; ny = ny_; nz = nz_;
    sx = sy = sz = 1.0f;
    data_.resize(static_cast<std::size_t>(nx) * ny * nz);

    if (name == "ramp") {
        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    float val = (nx > 1) ? static_cast<float>(x) / (nx - 1) : 0.0f;
                    data_[static_cast<std::size_t>(x) + static_cast<std::size_t>(nx) * (static_cast<std::size_t>(y) + static_cast<std::size_t>(ny) * z)] =
                        static_cast<unsigned char>(std::clamp(std::round(val * 255.0f), 0.0f, 255.0f));
                }
            }
        }
    } else if (name == "sphere") {
        float cx = (nx - 1) * 0.5f;
        float cy = (ny - 1) * 0.5f;
        float cz = (nz - 1) * 0.5f;
        float maxR = std::sqrt(cx * cx + cy * cy + cz * cz);
        if (maxR <= 0.0001f) maxR = 1.0f;

        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    float dx = x - cx;
                    float dy = y - cy;
                    float dz = z - cz;
                    float dist = std::sqrt(dx * dx + dy * dy + dz * dz) / maxR;
                    float val = std::clamp(1.0f - dist, 0.0f, 1.0f);
                    data_[static_cast<std::size_t>(x) + static_cast<std::size_t>(nx) * (static_cast<std::size_t>(y) + static_cast<std::size_t>(ny) * z)] =
                        static_cast<unsigned char>(std::clamp(std::round(val * 255.0f), 0.0f, 255.0f));
                }
            }
        }
    } else if (name == "corner") {
        // Spec: 1 for x<32, y<12, z<8; else 0
        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    float val = (x < 32 && y < 12 && z < 8) ? 1.0f : 0.0f;
                    data_[static_cast<std::size_t>(x) + static_cast<std::size_t>(nx) * (static_cast<std::size_t>(y) + static_cast<std::size_t>(ny) * z)] =
                        static_cast<unsigned char>(std::clamp(std::round(val * 255.0f), 0.0f, 255.0f));
                }
            }
        }
    } else {
        std::cerr << "[Volume] Unknown synthetic dataset name: " << name << '\n';
        std::fill(data_.begin(), data_.end(), 0);
    }
}

float Volume::At(int i, int j, int k) const
{
    i = std::clamp(i, 0, nx - 1);
    j = std::clamp(j, 0, ny - 1);
    k = std::clamp(k, 0, nz - 1);
    std::size_t idx = static_cast<std::size_t>(i) + static_cast<std::size_t>(nx) * (static_cast<std::size_t>(j) + static_cast<std::size_t>(ny) * k);
    return static_cast<float>(data_[idx]) * (1.0f / 255.0f);
}

float Volume::Sample(float x, float y, float z) const
{
    // 1. Clamp each coordinate to [0, n-1]
    x = std::clamp(x, 0.0f, static_cast<float>(nx - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(ny - 1));
    z = std::clamp(z, 0.0f, static_cast<float>(nz - 1));

    // 2. i0 = floor, i1 = min(i0+1, n-1), f = coord - i0
    int x0 = static_cast<int>(std::floor(x));
    int x1 = std::min(x0 + 1, nx - 1);
    float fx = x - static_cast<float>(x0);

    int y0 = static_cast<int>(std::floor(y));
    int y1 = std::min(y0 + 1, ny - 1);
    float fy = y - static_cast<float>(y0);

    int z0 = static_cast<int>(std::floor(z));
    int z1 = std::min(z0 + 1, nz - 1);
    float fz = z - static_cast<float>(z0);

    // 3. Fetch 8 neighbours
    float v000 = At(x0, y0, z0);
    float v100 = At(x1, y0, z0);
    float v010 = At(x0, y1, z0);
    float v110 = At(x1, y1, z0);
    float v001 = At(x0, y0, z1);
    float v101 = At(x1, y0, z1);
    float v011 = At(x0, y1, z1);
    float v111 = At(x1, y1, z1);

    // Lerp along x (4)
    float c00 = v000 + fx * (v100 - v000);
    float c10 = v010 + fx * (v110 - v010);
    float c01 = v001 + fx * (v101 - v001);
    float c11 = v011 + fx * (v111 - v011);

    // Lerp along y (2)
    float c0 = c00 + fy * (c10 - c00);
    float c1 = c01 + fy * (c11 - c01);

    // Lerp along z (1)
    return c0 + fz * (c1 - c0);
}

unsigned int Volume::CreateGLTexture3D() const
{
    if (nx <= 0 || ny <= 0 || nz <= 0 || data_.empty()) {
        return 0;
    }

    GLint max3D = 2048;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3D);
    if (max3D <= 0) max3D = 2048;

    // Check if texture dimensions exceed GPU limits or 512 MB VRAM threshold
    int stride = 1;
    int texNx = nx, texNy = ny, texNz = nz;
    while (texNx > max3D || texNy > max3D || texNz > max3D ||
           (static_cast<std::size_t>(texNx) * texNy * texNz > 512ULL * 1024 * 1024)) {
        stride *= 2;
        texNx = (nx + stride - 1) / stride;
        texNy = (ny + stride - 1) / stride;
        texNz = (nz + stride - 1) / stride;
    }

    std::vector<unsigned char> downsampled;
    const unsigned char* uploadData = data_.data();

    if (stride > 1) {
        std::cout << "[GPU] Volume (" << nx << "x" << ny << "x" << nz
                  << ", " << (static_cast<std::size_t>(nx) * ny * nz) / (1024 * 1024)
                  << " MB) exceeds 3D texture limit (" << max3D << " max dim / 512 MB VRAM cap).\n";
        std::cout << "[GPU] Subsampling " << stride << "x -> (" << texNx << "x" << texNy << "x" << texNz
                  << ", " << (static_cast<std::size_t>(texNx) * texNy * texNz) / (1024 * 1024)
                  << " MB) for GPU 3D texture.\n";

        downsampled.resize(static_cast<std::size_t>(texNx) * texNy * texNz);
        for (int z = 0; z < texNz; ++z) {
            int srcZ = std::min(z * stride, nz - 1);
            for (int y = 0; y < texNy; ++y) {
                int srcY = std::min(y * stride, ny - 1);
                for (int x = 0; x < texNx; ++x) {
                    int srcX = std::min(x * stride, nx - 1);
                    downsampled[x + texNx * (y + texNy * z)] =
                        data_[srcX + nx * (srcY + ny * srcZ)];
                }
            }
        }
        uploadData = downsampled.data();
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, texNx, texNy, texNz, 0, GL_RED, GL_UNSIGNED_BYTE, uploadData);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[GPU] glTexImage3D failed with error: " << err << '\n';
        glDeleteTextures(1, &tex);
        return 0;
    }

    glBindTexture(GL_TEXTURE_3D, 0);
    return tex;
}
