#include "colourmap.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>

struct ColorPoint {
    float x;
    Vector3f rgb;
};

static const ColorPoint* GetControlPoints(int& count) {
    static const ColorPoint points[] = {
        { 0.0f,      Vector3f(0.0862745098039216f, 0.00392156862745098f, 0.298039215686275f) },
        { 0.030334f, Vector3f(0.113725f,           0.0235294f,           0.45098f) },
        { 0.055527f, Vector3f(0.105882f,           0.0509804f,           0.509804f) },
        { 0.073008f, Vector3f(0.0392157f,          0.0392157f,           0.560784f) },
        { 0.089974f, Vector3f(0.0313725f,          0.0980392f,           0.6f) },
        { 0.106427f, Vector3f(0.0431373f,          0.164706f,            0.639216f) },
        { 0.130077f, Vector3f(0.054902f,           0.243137f,            0.678431f) },
        { 0.16144f,  Vector3f(0.054902f,           0.317647f,            0.709804f) },
        { 0.2f,      Vector3f(0.0509804f,          0.396078f,            0.741176f) },
        { 0.225f,    Vector3f(0.0392157f,          0.466667f,            0.768627f) },
        { 0.25f,     Vector3f(0.0313725f,          0.537255f,            0.788235f) },
        { 0.276093f, Vector3f(0.0313725f,          0.615686f,            0.811765f) },
        { 0.302828f, Vector3f(0.0235294f,          0.709804f,            0.831373f) },
        { 0.329563f, Vector3f(0.0509804f,          0.8f,                 0.85098f) },
        { 0.351671f, Vector3f(0.0705882f,          0.854902f,            0.870588f) },
        { 0.372237f, Vector3f(0.262745f,           0.901961f,            0.862745f) },
        { 0.390231f, Vector3f(0.423529f,           0.941176f,            0.87451f) },
        { 0.417995f, Vector3f(0.572549f,           0.964706f,            0.835294f) },
        { 0.436504f, Vector3f(0.658824f,           0.980392f,            0.843137f) },
        { 0.456041f, Vector3f(0.764706f,           0.980392f,            0.866667f) },
        { 0.468895f, Vector3f(0.827451f,           0.980392f,            0.886275f) },
        { 0.482262f, Vector3f(0.890196078431372f,  0.988235294117647f,   0.925490196078431f) },
        { 0.492545f, Vector3f(0.913725f,           0.988235f,            0.937255f) },
        { 0.501285f, Vector3f(1.0f,                1.0f,                 0.972549019607843f) },
        { 0.510026f, Vector3f(0.988235294117647f,  0.988235294117647f,   0.905882352941176f) },
        { 0.526478f, Vector3f(0.992156862745098f,  0.972549019607843f,   0.803921568627451f) },
        { 0.539846f, Vector3f(0.992157f,           0.964706f,            0.713725f) },
        { 0.554756f, Vector3f(0.988235f,           0.956863f,            0.643137f) },
        { 0.576864f, Vector3f(0.980392f,           0.917647f,            0.509804f) },
        { 0.599486f, Vector3f(0.968627f,           0.87451f,             0.407843f) },
        { 0.620051f, Vector3f(0.94902f,            0.823529f,            0.321569f) },
        { 0.636504f, Vector3f(0.929412f,           0.776471f,            0.278431f) },
        { 0.660668f, Vector3f(0.909804f,           0.717647f,            0.235294f) },
        { 0.682262f, Vector3f(0.890196f,           0.658824f,            0.196078f) },
        { 0.7f,      Vector3f(0.878431f,           0.619608f,            0.168627f) },
        { 0.725f,    Vector3f(0.870588f,           0.54902f,             0.156863f) },
        { 0.75f,     Vector3f(0.85098f,            0.47451f,             0.145098f) },
        { 0.775f,    Vector3f(0.831373f,           0.411765f,            0.133333f) },
        { 0.8f,      Vector3f(0.811765f,           0.345098f,            0.113725f) },
        { 0.825f,    Vector3f(0.788235f,           0.266667f,            0.0941176f) },
        { 0.85f,     Vector3f(0.741176f,           0.184314f,            0.0745098f) },
        { 0.875f,    Vector3f(0.690196f,           0.12549f,             0.0627451f) },
        { 0.9f,      Vector3f(0.619608f,           0.0627451f,           0.0431373f) },
        { 0.923393f, Vector3f(0.54902f,            0.027451f,            0.0705882f) },
        { 0.943959f, Vector3f(0.470588f,           0.0156863f,           0.0901961f) },
        { 0.967095f, Vector3f(0.4f,                0.00392157f,          0.101961f) },
        { 1.0f,      Vector3f(0.188235294117647f,  0.0f,                 0.0705882352941176f) }
    };
    count = static_cast<int>(sizeof(points) / sizeof(points[0]));
    return points;
}

static inline float lerp1f(float a, float b, float t) {
    return a + t * (b - a);
}

static Vector3f LerpColor(const Vector3f& a, const Vector3f& b, float t) {
    return Vector3f(lerp1f(a.x, b.x, t), lerp1f(a.y, b.y, t), lerp1f(a.z, b.z, t));
}

void Colourmap::Rebuild()
{
    int count = 0;
    const ColorPoint* points = GetControlPoints(count);

    for (int i = 0; i < ColormapSize; ++i) {
        float t = static_cast<float>(i) / (ColormapSize - 1.0f); // [0, 1]

        if (t <= points[0].x) {
            table_[i] = points[0].rgb;
        } else if (t >= points[count - 1].x) {
            table_[i] = points[count - 1].rgb;
        } else {
            // Find segment [k, k+1]
            int k = 0;
            for (int seg = 0; seg < count - 1; ++seg) {
                if (t >= points[seg].x && t <= points[seg + 1].x) {
                    k = seg;
                    break;
                }
            }
            float span = points[k + 1].x - points[k].x;
            float frac = (span > 0.0f) ? (t - points[k].x) / span : 0.0f;
            table_[i] = LerpColor(points[k].rgb, points[k + 1].rgb, frac);
        }
    }
}

Colourmap::Colourmap()
{
    Rebuild();
}

Vector3f Colourmap::Lookup(float s) const
{
    float normS = std::max(0.0f, std::min(1.0f, s));
    float p = normS * (ColormapSize - 1.0f);

    int idx0 = static_cast<int>(std::floor(p));
    int idx1 = std::min(idx0 + 1, ColormapSize - 1);
    float frac = p - static_cast<float>(idx0);

    return LerpColor(table_[idx0], table_[idx1], frac);
}

unsigned int Colourmap::CreateGLTexture1D() const
{
    std::vector<unsigned char> rgba(ColormapSize * 4);
    for (int i = 0; i < ColormapSize; ++i) {
        float r = std::max(0.0f, std::min(1.0f, table_[i].x));
        float g = std::max(0.0f, std::min(1.0f, table_[i].y));
        float b = std::max(0.0f, std::min(1.0f, table_[i].z));
        rgba[i * 4 + 0] = static_cast<unsigned char>(std::round(r * 255.0f));
        rgba[i * 4 + 1] = static_cast<unsigned char>(std::round(g * 255.0f));
        rgba[i * 4 + 2] = static_cast<unsigned char>(std::round(b * 255.0f));
        rgba[i * 4 + 3] = 255;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_1D, tex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, ColormapSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_1D, 0);

    return tex;
}

