#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "ogldev_math_3d.h"
#include "ogldev_util.h"
#include "world.h"
#include "view.h"
#include "project.h"
#include "volume.h"
#include "colourmap.h"
#include "slicing.h"

// ── Global Constants ─────────────────────────────────────────────────────────

static float WINDOW_WIDTH  = 1920.0f;
static float WINDOW_HEIGHT = 1080.0f;
static const char* TheProgramTitle = "Assignment 2 - Task 1: Volume Slicing";

static const float CameraFOV = 45.0f;
static const float CameraNear = 0.1f;
static const float CameraFar = 20.0f;

static const float ScalingFactor = 2.0f;
static const Vector3f CameraPosition(0.0f, 0.0f, 5.0f);
static const Vector3f CameraForward(0.0f, 0.0f, -1.0f);
static const Vector3f CameraUp(0.0f, 1.0f, 0.0f);

static const float BgColorR = 0.1f;
static const float BgColorG = 0.1f;
static const float BgColorB = 0.1f;
static const float BgColorA = 1.0f;

// ── Globals ──────────────────────────────────────────────────────────────────

static World ModelWorld;
static View ModelView;
static Project ModelProject(CameraFOV, WINDOW_WIDTH, WINDOW_HEIGHT, CameraNear, CameraFar);

static Volume gVolume;
static Colourmap gColormap;
static Slicing SliceController;

struct SliceProgram {
    GLuint id = 0;
    GLint world = -1;
    GLint view = -1;
    GLint projection = -1;
};

static SliceProgram CpuProg;
static SliceProgram GpuProg;

static bool gUseGpuMode = false; // Start in CPU mode (Part 1); 'm' toggles to GPU mode (Part 2)
static GLuint gVolume3DTexture = 0;
static GLuint gColormap1DTexture = 0;

static int gFpsFrameCount = 0;
static float gFPS = 0.0f;
static float gFrameTimeMs = 0.0f;
static auto gLastFpsTime = std::chrono::high_resolution_clock::now();
static auto gLastFrameTime = std::chrono::high_resolution_clock::now();

static GLuint VAO = 0;
static GLuint VBO = 0;
static GLuint IBO = 0;
static GLuint SliceTextures[3] = { 0, 0, 0 };

static bool IsFullScreen = false;
static float WindowPositionX = 0.0f;
static float WindowPositionY = 0.0f;

static char keyHandled = 'Z';

static void CheckGLError(const char* label)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[GL Error] " << label << ": " << err << '\n';
    }
}

static void CheckUniformLocation(GLint location, const char* name)
{
    if (location == -1) {
        std::cerr << "[Shader] Error getting uniform location of '" << name << "'\n";
        std::exit(EXIT_FAILURE);
    }
}

static void AddShader(GLuint program, const char* pShaderText, GLenum shaderType)
{
    GLuint shaderObj = glCreateShader(shaderType);
    if (shaderObj == 0) {
        std::cerr << "Error creating shader type " << shaderType << '\n';
        std::exit(EXIT_FAILURE);
    }

    const GLchar* p[1];
    p[0] = pShaderText;
    GLint lengths[1];
    lengths[0] = static_cast<GLint>(std::strlen(pShaderText));

    glShaderSource(shaderObj, 1, p, lengths);
    glCompileShader(shaderObj);

    GLint success;
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shaderObj, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Error compiling shader type " << shaderType << ": '" << infoLog << "'\n";
        std::exit(EXIT_FAILURE);
    }

    glAttachShader(program, shaderObj);
}

static GLuint CompileProgram(const char* vsFileName, const char* fsFileName)
{
    GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Error creating shader program\n";
        std::exit(EXIT_FAILURE);
    }

    std::string vsText, fsText;
    if (!ReadFile(vsFileName, vsText)) {
        std::cerr << "Failed to read " << vsFileName << '\n';
        std::exit(EXIT_FAILURE);
    }
    if (!ReadFile(fsFileName, fsText)) {
        std::cerr << "Failed to read " << fsFileName << '\n';
        std::exit(EXIT_FAILURE);
    }

    AddShader(program, vsText.c_str(), GL_VERTEX_SHADER);
    AddShader(program, fsText.c_str(), GL_FRAGMENT_SHADER);

    glLinkProgram(program);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar errorLog[1024];
        glGetProgramInfoLog(program, sizeof(errorLog), nullptr, errorLog);
        std::cerr << "Error linking shader program: '" << errorLog << "'\n";
        std::exit(EXIT_FAILURE);
    }

    return program;
}

static SliceProgram LoadSliceProgram(const char* vsFileName, const char* fsFileName)
{
    SliceProgram p;
    p.id = CompileProgram(vsFileName, fsFileName);
    p.world = glGetUniformLocation(p.id, "gWorld");
    CheckUniformLocation(p.world, "gWorld");
    p.view = glGetUniformLocation(p.id, "gView");
    CheckUniformLocation(p.view, "gView");
    p.projection = glGetUniformLocation(p.id, "gProjection");
    CheckUniformLocation(p.projection, "gProjection");
    return p;
}

static void UpdateWindowTitle()
{
    char title[256];
    std::snprintf(title, sizeof(title),
        "%s [%s] | %.1f FPS (%.2f ms) | X: %d/%d (%s)  Y: %d/%d (%s)  Z: %d/%d (%s) | %s",
        TheProgramTitle,
        gUseGpuMode ? "GPU (3D Tex)" : "CPU (2D Slices)",
        gFPS, gFrameTimeMs,
        SliceController.GetSliceIndex(SliceAxis::X, gVolume), gVolume.nx - 1,
        SliceController.IsVisible(SliceAxis::X) ? "ON" : "OFF",
        SliceController.GetSliceIndex(SliceAxis::Y, gVolume), gVolume.ny - 1,
        SliceController.IsVisible(SliceAxis::Y) ? "ON" : "OFF",
        SliceController.GetSliceIndex(SliceAxis::Z, gVolume), gVolume.nz - 1,
        SliceController.IsVisible(SliceAxis::Z) ? "ON" : "OFF",
        ModelProject.IsOrthographic() ? "Ortho" : "Persp");
    glutSetWindowTitle(title);
}

static void PrintControls()
{
    std::cout << "\n======================================================\n";
    std::cout << " Volume Slicing - Controls (CPU & GPU)\n";
    std::cout << "======================================================\n";
    std::cout << " Pipeline Mode:\n";
    std::cout << "   m / M       : Toggle CPU Slicing <-> GPU Slicing\n";
    std::cout << "   b / B       : Run 50-step slicing benchmark (reports FPS & latency)\n\n";
    std::cout << " Slices:\n";
    std::cout << "   x / X       : Move X slice - / + 1 voxel (Red plane)\n";
    std::cout << "   y / Y       : Move Y slice - / + 1 voxel (Green plane)\n";
    std::cout << "   z / Z       : Move Z slice - / + 1 voxel (Blue plane)\n";
    std::cout << "   7 / 8 / 9   : Toggle visibility of X / Y / Z slice\n\n";
    std::cout << " Model Rotation:\n";
    std::cout << "   1 / 2       : Rotate around X axis (+ / -)\n";
    std::cout << "   3 / 4       : Rotate around Y axis (+ / -)\n";
    std::cout << "   5 / 6       : Rotate around Z axis (+ / -)\n";
    std::cout << "   0           : Reset model transform\n\n";
    std::cout << " Face-on Presets (for screenshots):\n";
    std::cout << "   q           : Face-on XY slice (Z fixed)\n";
    std::cout << "   w           : Face-on YZ slice (X fixed)\n";
    std::cout << "   e           : Face-on ZX slice (Y fixed)\n\n";
    std::cout << " Camera & View:\n";
    std::cout << "   i / k       : Camera move forward / backward\n";
    std::cout << "   j / l       : Camera move left / right\n";
    std::cout << "   u / o       : Camera move up / down\n";
    std::cout << "   p / P       : Toggle perspective / orthographic\n";
    std::cout << "   Arrow Keys  : Translate model\n";
    std::cout << "   F1          : Toggle fullscreen\n";
    std::cout << "   ESC         : Exit program\n";
    std::cout << "======================================================\n\n";
}

// ── Verification tests ───────────────────────────────────────────────────────

static void RunSyntheticVerifications()
{
    std::cout << "[Verify] Running pre-flight unit checks...\n";

    // 1. Ramp check
    Volume ramp;
    ramp.CreateSynthetic("ramp", 64, 48, 32);
    float s0 = ramp.Sample(0.0f, 0.0f, 0.0f);
    float sMax = ramp.Sample(static_cast<float>(ramp.nx - 1), 0.0f, 0.0f);
    float sHalf = ramp.Sample(0.5f, 0.0f, 0.0f);
    std::cout << "  Ramp check: Sample(0,0,0)=" << s0
              << ", Sample(nx-1,0,0)=" << sMax
              << ", Sample(0.5,0,0)=" << sHalf
              << " (expected: 0, 1, " << (0.5f / (ramp.nx - 1)) << ")\n";

    // 2. Colormap lookup check
    Colourmap cm;
    Vector3f c0 = cm.Lookup(0.0f);
    Vector3f cMid = cm.Lookup(0.5f);
    Vector3f c1 = cm.Lookup(1.0f);
    std::printf("  Colormap check: Lookup(0.0)=(%.3f, %.3f, %.3f), Lookup(0.5)=(%.3f, %.3f, %.3f), Lookup(1.0)=(%.3f, %.3f, %.3f)\n",
                c0.x, c0.y, c0.z, cMid.x, cMid.y, cMid.z, c1.x, c1.y, c1.z);

    // 3. Corner volume check (spec: Z: 32x12=384, X: 12x8=96, Y: 8x32=256)
    Volume corner;
    corner.CreateSynthetic("corner", 64, 48, 32);
    Slicing cornerSlicing;
    // Set slices to t=0.1 so each slice is strictly inside the corner block:
    // x = 0.1 * 63 = 6.3 < 32; y = 0.1 * 47 = 4.7 < 12; z = 0.1 * 31 = 3.1 < 8
    cornerSlicing.SetPosition(SliceAxis::X, 0.1f);
    cornerSlicing.SetPosition(SliceAxis::Y, 0.1f);
    cornerSlicing.SetPosition(SliceAxis::Z, 0.1f);

    auto countBrightPixels = [&](SliceAxis axis) {
        SliceImage img = cornerSlicing.BuildSliceImage(corner, cm, axis);
        int bright = 0;
        Vector3f cBg = cm.Lookup(0.0f);
        int bgR = static_cast<int>(std::round(cBg.x * 255.0f));
        int bgG = static_cast<int>(std::round(cBg.y * 255.0f));
        int bgB = static_cast<int>(std::round(cBg.z * 255.0f));
        for (std::size_t i = 0; i < img.pixels.size(); i += 4) {
            if (std::abs(static_cast<int>(img.pixels[i]) - bgR) > 5 ||
                std::abs(static_cast<int>(img.pixels[i + 1]) - bgG) > 5 ||
                std::abs(static_cast<int>(img.pixels[i + 2]) - bgB) > 5) {
                bright++;
            }
        }
        return std::make_pair(img, bright);
    };

    auto resZ = countBrightPixels(SliceAxis::Z);
    auto resX = countBrightPixels(SliceAxis::X);
    auto resY = countBrightPixels(SliceAxis::Y);

    std::cout << "  Corner check:\n";
    std::cout << "    Z slice: " << resZ.first.width << "x" << resZ.first.height
              << " bright=" << resZ.second << " (expected: 384)\n";
    std::cout << "    X slice: " << resX.first.width << "x" << resX.first.height
              << " bright=" << resX.second << " (expected: 96)\n";
    std::cout << "    Y slice: " << resY.first.width << "x" << resY.first.height
              << " bright=" << resY.second << " (expected: 256)\n";
}

// ── Rendering & GLUT Callbacks ───────────────────────────────────────────────

static void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool anyDirty = SliceController.IsDirty(SliceAxis::X) ||
                    SliceController.IsDirty(SliceAxis::Y) ||
                    SliceController.IsDirty(SliceAxis::Z);

    if (anyDirty) {
        if (!gUseGpuMode) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            for (int axis = 0; axis < 3; ++axis) {
                SliceAxis a = static_cast<SliceAxis>(axis);
                if (SliceController.IsDirty(a)) {
                    SliceImage img = SliceController.BuildSliceImage(gVolume, gColormap, a);
                    glBindTexture(GL_TEXTURE_2D, SliceTextures[axis]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                                 img.width, img.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE,
                                 img.pixels.data());
                    glBindTexture(GL_TEXTURE_2D, 0);
                    SliceController.ClearDirty(a);
                }
            }
        }

        std::vector<QuadVertex> quadVerts = SliceController.BuildQuadVertices(gVolume);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, quadVerts.size() * sizeof(QuadVertex), quadVerts.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (gUseGpuMode) {
            SliceController.ClearDirty(SliceAxis::X);
            SliceController.ClearDirty(SliceAxis::Y);
            SliceController.ClearDirty(SliceAxis::Z);
        }
    }

    const SliceProgram& prog = gUseGpuMode ? GpuProg : CpuProg;
    glUseProgram(prog.id);

    Matrix4f W = ModelWorld.GetMatrix();
    Matrix4f V = ModelView.GetMatrix();
    Matrix4f P = ModelProject.GetMatrix();

    glUniformMatrix4fv(prog.world, 1, GL_TRUE, &W.m[0][0]);
    glUniformMatrix4fv(prog.view, 1, GL_TRUE, &V.m[0][0]);
    glUniformMatrix4fv(prog.projection, 1, GL_TRUE, &P.m[0][0]);

    if (gUseGpuMode) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, gVolume3DTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, gColormap1DTexture);
    }

    glBindVertexArray(VAO);
    for (int axis = 0; axis < 3; ++axis) {
        SliceAxis a = static_cast<SliceAxis>(axis);
        if (!SliceController.IsVisible(a)) continue;

        if (!gUseGpuMode) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, SliceTextures[axis]);
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(axis * 6 * sizeof(unsigned int)));
    }
    glBindVertexArray(0);
    glUseProgram(0);

    CheckGLError("RenderSceneCB");

    // FPS calculation
    auto now = std::chrono::high_resolution_clock::now();
    gFrameTimeMs = std::chrono::duration<float, std::milli>(now - gLastFrameTime).count();
    gLastFrameTime = now;
    gFpsFrameCount++;

    float elapsedSec = std::chrono::duration<float>(now - gLastFpsTime).count();
    if (elapsedSec >= 0.5f) {
        gFPS = static_cast<float>(gFpsFrameCount) / elapsedSec;
        gFpsFrameCount = 0;
        gLastFpsTime = now;
        UpdateWindowTitle();
    }

    glutSwapBuffers();
}

static void onReshape(int width, int height)
{
    glViewport(0, 0, width, height);
    if (!IsFullScreen) {
        WINDOW_WIDTH = static_cast<float>(width);
        WINDOW_HEIGHT = static_cast<float>(height);
    }
    ModelProject.SetViewportSize(static_cast<float>(width), static_cast<float>(height));
}

static void RunBenchmark()
{
    const int stepCount = 50;
    std::cout << "\n[Benchmark] Running " << stepCount << " slice steps in "
              << (gUseGpuMode ? "GPU (3D Texture)" : "CPU (2D Slices)") << " mode...\n";

    // Warm-up and synchronization
    for (int i = 0; i < 5; ++i) {
        SliceController.HandleKey(keyHandled, gVolume);
        RenderSceneCB();
    }
    glFinish();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < stepCount; ++i) {
        SliceController.HandleKey(keyHandled, gVolume);
        RenderSceneCB();
    }
    glFinish();

    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgMs = totalMs / stepCount;
    double fps = 1000.0 / (avgMs > 0.001 ? avgMs : 0.001);

    std::printf("[Benchmark] %s: %d steps took %.2f ms (%.2f ms/step, ~%.1f FPS)\n\n",
                gUseGpuMode ? "GPU (3D Tex)" : "CPU (2D Slices)",
                stepCount, totalMs, avgMs, fps);
}

static void IdleCB()
{
    glutPostRedisplay();
}

static void KeyboardCB(unsigned char key, int mouse_x, int mouse_y)
{
    const float rotationStep = 5.0f;

    // Step 7: First call SliceController.HandleKey(...)
    if (SliceController.HandleKey(key, gVolume)) {
        UpdateWindowTitle();
        glutPostRedisplay();
        return;
    }

    switch (key) {
        case '1':
            ModelWorld.Rotate(rotationStep, 0.0f, 0.0f);
            break;
        case '2':
            ModelWorld.Rotate(-rotationStep, 0.0f, 0.0f);
            break;
        case '3':
            ModelWorld.Rotate(0.0f, rotationStep, 0.0f);
            break;
        case '4':
            ModelWorld.Rotate(0.0f, -rotationStep, 0.0f);
            break;
        case '5':
            ModelWorld.Rotate(0.0f, 0.0f, rotationStep);
            break;
        case '6':
            ModelWorld.Rotate(0.0f, 0.0f, -rotationStep);
            break;

        case '0':
            ModelWorld.ResetTransform();
            break;

        case 'm':
        case 'M':
            if (gVolume3DTexture == 0) {
                std::cout << "[Pipeline] Cannot switch to GPU mode: 3D texture is not available.\n";
                break;
            }
            gUseGpuMode = !gUseGpuMode;
            SliceController.MarkAllDirty();
            UpdateWindowTitle();
            std::cout << "[Pipeline] Switched to " << (gUseGpuMode ? "GPU Slicing (3D Texture)" : "CPU Slicing (2D Textures)") << '\n';
            break;

        case 'b':
        case 'B':
            RunBenchmark();
            break;

        case 'p':
        case 'P':
            ModelProject.ToggleProjection();
            break;

        // Preset face-on views:
        case 'q':
        case 'Q':
            ModelWorld.ResetTransform(); // XY slice face-on
            keyHandled = 'Z';
            break;

        case 'w':
        case 'W':
            ModelWorld.ResetTransform();
            ModelWorld.Rotate(0.0f, 90.0f, 0.0f); // YZ slice face-on
            keyHandled = 'x';
            break;

        case 'e':
        case 'E':
            ModelWorld.ResetTransform();
            ModelWorld.Rotate(90.0f, 0.0f, 0.0f); // ZX slice face-on
            keyHandled = 'Y';
            break;

        case 27: // ESC
            glutDestroyWindow(glutGetWindow());
            std::exit(0);

        default:
            ModelView.OnKeyboard(key);
            break;
    }

    UpdateWindowTitle();
    glutPostRedisplay();
}

static void SpecialKeyboardCB(int key, int mouse_x, int mouse_y)
{
    const float moveStep = 0.1f;

    switch (key) {
        case GLUT_KEY_LEFT:
            ModelWorld.Translate(-moveStep, 0.0f, 0.0f);
            break;
        case GLUT_KEY_RIGHT:
            ModelWorld.Translate(moveStep, 0.0f, 0.0f);
            break;
        case GLUT_KEY_UP:
            ModelWorld.Translate(0.0f, moveStep, 0.0f);
            break;
        case GLUT_KEY_DOWN:
            ModelWorld.Translate(0.0f, -moveStep, 0.0f);
            break;
        case GLUT_KEY_F1:
            IsFullScreen = !IsFullScreen;
            if (IsFullScreen) {
                WindowPositionX = static_cast<float>(glutGet(GLUT_WINDOW_X));
                WindowPositionY = static_cast<float>(glutGet(GLUT_WINDOW_Y));
                glutFullScreen();
            } else {
                glutReshapeWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT));
                glutPositionWindow(static_cast<int>(WindowPositionX), static_cast<int>(WindowPositionY));
            }
            break;
        default:
            break;
    }

    glutPostRedisplay();
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    // Command line parsing:
    // Test:       ./main --test
    // Synthetic:  ./main --synthetic ramp|sphere|corner
    // Real data:  ./main <path.raw> [nx ny nz]
    if (argc >= 2 && std::strcmp(argv[1], "--test") == 0) {
        RunSyntheticVerifications();
        return 0;
    }

    if (argc >= 2 && std::strcmp(argv[1], "--synthetic") == 0) {
        std::string synthName = (argc >= 3) ? argv[2] : "ramp";
        std::cout << "[main] Loading synthetic dataset: " << synthName << " (64x48x32)\n";
        gVolume.CreateSynthetic(synthName, 64, 48, 32);
    } else if (argc >= 2) {
        std::string path = argv[1];
        int nx = 0, ny = 0, nz = 0;
        if (argc >= 5) {
            nx = std::atoi(argv[2]);
            ny = std::atoi(argv[3]);
            nz = std::atoi(argv[4]);
        } else {
            // Auto-detect dimensions from filename if not specified (e.g. name_512x512x361_uint16.raw)
            for (std::size_t i = 0; i < path.size(); ++i) {
                if (std::sscanf(path.c_str() + i, "%dx%dx%d", &nx, &ny, &nz) == 3) {
                    break;
                }
            }
            if (nx <= 0 || ny <= 0 || nz <= 0) {
                nx = 256; ny = 256; nz = 256;
            }
        }
        std::cout << "[main] Loading raw volume: " << path << " (" << nx << "x" << ny << "x" << nz << ")\n";
        if (!gVolume.LoadRaw(path, nx, ny, nz)) {
            std::cerr << "Failed to load volume: " << path << '\n';
            return 1;
        }
    } else {
        std::cout << "Usage:\n";
        std::cout << "  Real volume: " << argv[0] << " <path.raw> [nx ny nz]\n";
        std::cout << "  Synthetic  : " << argv[0] << " --synthetic ramp|sphere|corner\n";
        std::cout << "  Unit tests : " << argv[0] << " --test\n";
        std::cout << "Defaulting to data/bonsai_256x256x256_uint8.raw 256 256 256\n";
        if (!gVolume.LoadRaw("data/bonsai_256x256x256_uint8.raw", 256, 256, 256)) {
            std::cout << "Fallback to synthetic sphere (64x48x32)\n";
            gVolume.CreateSynthetic("sphere", 64, 48, 32);
        }
    }

    // Step 6.1: GLUT setup
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT));
    glutInitWindowPosition(50, 50);
    glutCreateWindow(TheProgramTitle);

    GLenum res = glewInit();
    if (res != GLEW_OK) {
        std::cerr << "glewInit error: " << glewGetErrorString(res) << '\n';
        return 1;
    }

    // Enable depth test, disable culling
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(BgColorR, BgColorG, BgColorB, BgColorA);

    // Step 6.2: One VBO (12 QuadVertex), one static IBO (18 indices), one VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(QuadVertex), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    const auto& indices = Slicing::GetQuadIndices();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Attribute 0: position (3 floats), offset 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          reinterpret_cast<void*>(offsetof(QuadVertex, position)));

    // Attribute 1: uv (2 floats), offset 12
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          reinterpret_cast<void*>(offsetof(QuadVertex, u)));

    // Attribute 2: texCoord3D (3 floats), offset 20
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          reinterpret_cast<void*>(offsetof(QuadVertex, texCoord3D)));

    // Upload initial quad vertices
    std::vector<QuadVertex> initialVerts = SliceController.BuildQuadVertices(gVolume);
    glBufferSubData(GL_ARRAY_BUFFER, 0, initialVerts.size() * sizeof(QuadVertex), initialVerts.data());

    glBindVertexArray(0);

    // Step 6.3: Three GL_TEXTURE_2D objects (SliceTextures[3])
    glGenTextures(3, SliceTextures);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_2D, SliceTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Step 6.4: ModelWorld normalization and camera setup
    float Ex = gVolume.GetExtent(0);
    float Ey = gVolume.GetExtent(1);
    float Ez = gVolume.GetExtent(2);
    Vector3f centre(Ex * 0.5f, Ey * 0.5f, Ez * 0.5f);
    float maxExtent = std::max(Ex, std::max(Ey, Ez));
    if (maxExtent <= 0.0001f) maxExtent = 1.0f;

    ModelWorld.SetNormalization(centre, ScalingFactor / maxExtent);
    ModelWorld.ResetTransform();

    ModelView.SetPosition(CameraPosition.x, CameraPosition.y, CameraPosition.z);
    ModelView.SetForward(CameraForward.x, CameraForward.y, CameraForward.z);
    ModelView.SetUp(CameraUp.x, CameraUp.y, CameraUp.z);

    // Step 5: Compile CPU and GPU slice programs
    CpuProg = LoadSliceProgram("slice.vs", "slice.fs");
    GpuProg = LoadSliceProgram("slice_gpu.vs", "slice_gpu.fs");

    // Step 5b: GPU 3D volume texture and 1D colormap texture
    gVolume3DTexture = gVolume.CreateGLTexture3D();
    gColormap1DTexture = gColormap.CreateGLTexture1D();
    std::cout << "[GPU] Uploaded 3D Volume Texture (id=" << gVolume3DTexture 
              << ") and 1D Colormap Texture (id=" << gColormap1DTexture << ")\n";

    // Pre-bind sampler units once at startup
    GLint uVolume = glGetUniformLocation(GpuProg.id, "uVolume3D");
    GLint uColormap = glGetUniformLocation(GpuProg.id, "uColormap1D");
    glUseProgram(GpuProg.id);
    glUniform1i(uVolume, 0);   // uVolume3D -> Texture Unit 0
    glUniform1i(uColormap, 1); // uColormap1D -> Texture Unit 1
    glUseProgram(0);

    GLint uSliceImage = glGetUniformLocation(CpuProg.id, "gSliceImage");
    glUseProgram(CpuProg.id);
    glUniform1i(uSliceImage, 0); // gSliceImage -> Texture Unit 0
    glUseProgram(0);

    // Register GLUT callbacks
    glutDisplayFunc(RenderSceneCB);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(KeyboardCB);
    glutSpecialFunc(SpecialKeyboardCB);
    glutIdleFunc(IdleCB);

    UpdateWindowTitle();
    PrintControls();

    glutMainLoop();
    return 0;
}
