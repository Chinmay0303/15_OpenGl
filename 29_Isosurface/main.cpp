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

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include "ogldev_math_3d.h"
#include "ogldev_util.h"
#include "world.h"
#include "view.h"
#include "project.h"
#include "volume.h"
#include "colourmap.h"
#include "isohist.h"
#include "mc_tables.h"

// ═════════════════════════════════════════════════════════════════════════════
// ── DEMO CONFIGURATION & TUNING BLOCK ───────────────────────────────────────
// Readily adjustable parameters for evaluation and testing.
// ═════════════════════════════════════════════════════════════════════════════

// 1. Dataset & Fallbacks
static const char* DefaultDataPath           = "../28_Volume_Slicing/data/bonsai_256x256x256_uint8.raw";
static const int   DefaultDimsX              = 256;
static const int   DefaultDimsY              = 256;
static const int   DefaultDimsZ              = 256;

// 2. Isovalue Range & Presets
static const int   IsoMin                    = 1;
static const int   IsoMax                    = 255;
static int         IsoPresets[3]             = { 40, 80, 160 }; // Presets updated in Task 2.1
static const int   BaselineIso               = 0;
static const int   SweepStep                 = 16;

// 3. Benchmarking Parameters
static const int   BenchmarkWarmupFrames     = 5;
static const int   BenchmarkFrames           = 50;

// 4. Shading & Lighting
static const float AmbientLight              = 0.25f;
static const float DiffuseLight              = 0.75f;
static float SurfaceColor[3]                 = { 0.85f, 0.78f, 0.65f }; // Clean fixed bone/parchment tone

// 5. ImGui Panel Dimensions & Styling
static const float PanelWidth                = 620.0f;
static const float PanelMargin               = 10.0f;
static const float HistogramPlotWidth        = 512.0f;
static const float HistogramPlotHeight       = 160.0f;
static bool        HistogramLogScale         = true;

// 6. Peak Detection Tuning
static const int   PeakSmoothRadius          = 2; // 5-bin moving average
static const int   PeakCount                 = 5;
static const int   MinPeakSeparation         = 16;

// 7. Camera & Projection Defaults
static const float CameraFOV                 = 45.0f;
static const float CameraNear                = 0.1f;
static const float CameraFar                 = 20.0f;
static const float ScalingFactor             = 2.0f;
static const Vector3f CameraPosition(0.0f, 0.0f, 5.0f);
static const Vector3f CameraForward(0.0f, 0.0f, -1.0f);
static const Vector3f CameraUp(0.0f, 1.0f, 0.0f);

static const float BgColorR                  = 0.1f;
static const float BgColorG                  = 0.1f;
static const float BgColorB                  = 0.1f;
static const float BgColorA                  = 1.0f;

// 8. Interaction & Transformation Steps
static const float ModelRotationStepDeg      = 5.0f;
static const float ModelTranslateStep        = 0.1f;

// ═════════════════════════════════════════════════════════════════════════════
// ── Window & Runtime State ───────────────────────────────────────────────────
// ═════════════════════════════════════════════════════════════════════════════

static float WINDOW_WIDTH  = 1920.0f;
static float WINDOW_HEIGHT = 1080.0f;
static const char* TheProgramTitle = "Assignment 2 - Task 2: Data-Driven Isovalue Selection & Marching Cubes";

static World ModelWorld;
static View ModelView;
static Project ModelProject(CameraFOV, WINDOW_WIDTH, WINDOW_HEIGHT, CameraNear, CameraFar);

static Volume gVolume;
static Colourmap gColormap(MapType::DIVERGENT_1);
static int gIsoValue = 80;

// Histogram & Peak Detection State (Task 2.1)
static std::vector<long long> gHist(256, 0);
static float gHistPlot[256] = {0};
static float gHistPlotMax = 1.0f;
static std::vector<int> gDetectedPeaks;
static int gHistMaxK = 1;
static long long gHistMaxCells = 0;

static void RebuildHistPlot()
{
    gHistPlotMax = 0.0f;
    for (int k = 0; k < 256; ++k) {
        float val = 0.0f;
        if (k >= 1 && k < static_cast<int>(gHist.size())) {
            if (HistogramLogScale) {
                val = std::log10(1.0f + static_cast<float>(gHist[k]));
            } else {
                val = static_cast<float>(gHist[k]);
            }
        }
        gHistPlot[k] = val;
        if (val > gHistPlotMax) {
            gHistPlotMax = val;
        }
    }
    if (gHistPlotMax <= 0.0f) gHistPlotMax = 1.0f;
}

static bool IsFullScreen = false;
static int WindowPositionX = 50;
static int WindowPositionY = 50;

// ── GL Error & Shader Compilation Helpers ────────────────────────────────────

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

static GLuint CompileProgram(const char* vsFileName, const char* fsFileName, const char* gsFileName = nullptr)
{
    GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Error creating shader program\n";
        std::exit(EXIT_FAILURE);
    }

    std::string vsText, fsText, gsText;
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

    if (gsFileName) {
        if (!ReadFile(gsFileName, gsText)) {
            std::cerr << "Failed to read " << gsFileName << '\n';
            std::exit(EXIT_FAILURE);
        }
        AddShader(program, gsText.c_str(), GL_GEOMETRY_SHADER);
    }

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

// ── GPU Marching Cubes Pipeline (Task 2.2) ───────────────────────────────────

struct IsoProgramLocations {
    GLuint id = 0;
    GLint gWorld = -1;
    GLint gView = -1;
    GLint gProjection = -1;
    GLint uIso = -1;
    GLint uExtent = -1;
    GLint uColour = -1;
    GLint uAmbient = -1;
    GLint uDiffuse = -1;
};

static IsoProgramLocations gIsoProg;
static GLuint gEmptyVAO = 0;
static GLuint gVolumeTex = 0;
static GLuint gTriTableTex = 0;
static GLuint gQueryTris = 0;
static GLuint gQueryTime = 0;
static int gTexDimsX = 0, gTexDimsY = 0, gTexDimsZ = 0;
static int gNumCells = 0;
static GLuint64 gLastTris = 0;
static float gLastGpuMs = 0.0f;
static std::string gCpuGpuCheckResult = "Click 'Check CPU vs GPU' to compare hardware vs CPU triangle counts.";
static ImVec4 gCpuGpuCheckColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

static void InitIsoPipeline()
{
    gIsoProg.id = CompileProgram("iso.vs", "iso.fs", "iso.gs");

    gIsoProg.gWorld = glGetUniformLocation(gIsoProg.id, "gWorld");
    CheckUniformLocation(gIsoProg.gWorld, "gWorld");
    gIsoProg.gView = glGetUniformLocation(gIsoProg.id, "gView");
    CheckUniformLocation(gIsoProg.gView, "gView");
    gIsoProg.gProjection = glGetUniformLocation(gIsoProg.id, "gProjection");
    CheckUniformLocation(gIsoProg.gProjection, "gProjection");
    gIsoProg.uIso = glGetUniformLocation(gIsoProg.id, "uIso");
    CheckUniformLocation(gIsoProg.uIso, "uIso");
    gIsoProg.uExtent = glGetUniformLocation(gIsoProg.id, "uExtent");
    CheckUniformLocation(gIsoProg.uExtent, "uExtent");
    gIsoProg.uColour = glGetUniformLocation(gIsoProg.id, "uColour");
    CheckUniformLocation(gIsoProg.uColour, "uColour");
    gIsoProg.uAmbient = glGetUniformLocation(gIsoProg.id, "uAmbient");
    CheckUniformLocation(gIsoProg.uAmbient, "uAmbient");
    gIsoProg.uDiffuse = glGetUniformLocation(gIsoProg.id, "uDiffuse");
    CheckUniformLocation(gIsoProg.uDiffuse, "uDiffuse");

    glUseProgram(gIsoProg.id);
    glUniform1i(glGetUniformLocation(gIsoProg.id, "uVolume3D"), 0);
    glUniform1i(glGetUniformLocation(gIsoProg.id, "uTriTable"), 1);
    glUseProgram(0);

    // 1. Upload 3D Volume texture (Blueprint 8.2)
    gVolumeTex = gVolume.CreateGLTexture3D();
    glBindTexture(GL_TEXTURE_3D, gVolumeTex);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &gTexDimsX);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &gTexDimsY);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &gTexDimsZ);
    glBindTexture(GL_TEXTURE_3D, 0);

    gNumCells = std::max(0, gTexDimsX - 1) * std::max(0, gTexDimsY - 1) * std::max(0, gTexDimsZ - 1);
    std::cout << "[Iso] Texture dims: " << gTexDimsX << "x" << gTexDimsY << "x" << gTexDimsZ
              << ", rendering " << gNumCells << " cells.\n";

    // 2. Upload 16x256 GL_R8I integer texture for TriTable (Blueprint 8.3)
    glGenTextures(1, &gTriTableTex);
    glBindTexture(GL_TEXTURE_2D, gTriTableTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8I, 16, 256, 0, GL_RED_INTEGER, GL_BYTE, TriTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 3. Create empty VAO for vertexless point rendering (Blueprint 8.4)
    glGenVertexArrays(1, &gEmptyVAO);

    // 4. Create OpenGL query objects for primitives and time (Blueprint 8.9)
    glGenQueries(1, &gQueryTris);
    glGenQueries(1, &gQueryTime);

    CheckGLError("InitIsoPipeline");
}

static void DrawIso(int k, bool blocking = false)
{
    if (gNumCells <= 0 || gIsoProg.id == 0) return;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(gIsoProg.id);

    glUniformMatrix4fv(gIsoProg.gWorld, 1, GL_TRUE, (const GLfloat*)ModelWorld.GetMatrix());
    glUniformMatrix4fv(gIsoProg.gView, 1, GL_TRUE, (const GLfloat*)ModelView.GetMatrix());
    glUniformMatrix4fv(gIsoProg.gProjection, 1, GL_TRUE, (const GLfloat*)ModelProject.GetMatrix());

    // Decision D3: uIso = (k - 0.5) / 255.0
    float uIso = (static_cast<float>(k) - 0.5f) / 255.0f;
    glUniform1f(gIsoProg.uIso, uIso);

    Vector3f extent(gVolume.GetExtent(0), gVolume.GetExtent(1), gVolume.GetExtent(2));
    glUniform3f(gIsoProg.uExtent, extent.x, extent.y, extent.z);

    glUniform3f(gIsoProg.uColour, SurfaceColor[0], SurfaceColor[1], SurfaceColor[2]);
    glUniform1f(gIsoProg.uAmbient, AmbientLight);
    glUniform1f(gIsoProg.uDiffuse, DiffuseLight);

    // Rebind texture units every draw (ImGui alters texture bindings)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, gVolumeTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTriTableTex);

    glBindVertexArray(gEmptyVAO);

    glBeginQuery(GL_PRIMITIVES_GENERATED, gQueryTris);
    glBeginQuery(GL_TIME_ELAPSED, gQueryTime);

    glDrawArrays(GL_POINTS, 0, gNumCells);

    glEndQuery(GL_TIME_ELAPSED);
    glEndQuery(GL_PRIMITIVES_GENERATED);

    if (blocking) {
        glGetQueryObjectui64v(gQueryTris, GL_QUERY_RESULT, &gLastTris);
        GLuint64 timeNs = 0;
        glGetQueryObjectui64v(gQueryTime, GL_QUERY_RESULT, &timeNs);
        gLastGpuMs = static_cast<float>(timeNs) * 1e-6f;
    } else {
        GLint available = 0;
        glGetQueryObjectiv(gQueryTris, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            glGetQueryObjectui64v(gQueryTris, GL_QUERY_RESULT, &gLastTris);
            GLuint64 timeNs = 0;
            glGetQueryObjectui64v(gQueryTime, GL_QUERY_RESULT, &timeNs);
            gLastGpuMs = static_cast<float>(timeNs) * 1e-6f;
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// ── Benchmark & Quantitative Analysis (Task 2.3) ────────────────────────────

struct BenchRow {
    int k = 0;
    long long cells = 0;
    GLuint64 trisGpu = 0;
    long long trisCpu = 0;
    double trisPerCell = 0.0;
    float avgMs = 0.0f;
    double mtrisPerSec = 0.0;
    bool isPreset = false;
    bool isBaseline = false;
    bool isSweep = false;
};

static bool gBenchRunning = false;
static std::vector<int> gBenchList;
static int gBenchIdx = 0;
static int gBenchFrame = 0;
static float gBenchAccumMs = 0.0f;
static int gSavedIso = 80;
static std::vector<BenchRow> gBenchResults;

// Linear regression metrics
static float gFitA = 0.0f;
static float gFitB = 0.0f;
static float gFitR2 = 0.0f;
static float gBaselineMs = 0.0f;

// Stacked plot data over regular sweep
static std::vector<float> gPlotSweepK;
static std::vector<float> gPlotSweepCells;
static std::vector<float> gPlotSweepTris;
static std::vector<float> gPlotSweepGpuMs;
static float gPlotMaxCells = 1.0f;
static float gPlotMaxTris = 1.0f;
static float gPlotMaxGpuMs = 1.0f;

static bool FitLine(const std::vector<float>& x, const std::vector<float>& y, float& a, float& b, float& r2)
{
    std::size_t n = x.size();
    if (n < 2 || n != y.size()) return false;

    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        sx += x[i];
        sy += y[i];
        sxx += x[i] * x[i];
        sxy += x[i] * y[i];
    }

    double denom = n * sxx - sx * sx;
    if (std::abs(denom) < 1e-12) return false;

    b = static_cast<float>((n * sxy - sx * sy) / denom);
    a = static_cast<float>((sy - b * sx) / n);

    double yMean = sy / n;
    double ssTot = 0.0, ssRes = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        double f = a + b * x[i];
        ssRes += (y[i] - f) * (y[i] - f);
        ssTot += (y[i] - yMean) * (y[i] - yMean);
    }
    r2 = (ssTot > 1e-12) ? static_cast<float>(1.0 - (ssRes / ssTot)) : 1.0f;
    return true;
}

static void StartBenchmark()
{
    gSavedIso = gIsoValue;
    gBenchResults.clear();
    gPlotSweepK.clear();
    gPlotSweepCells.clear();
    gPlotSweepTris.clear();
    gPlotSweepGpuMs.clear();

    gBenchList.clear();
    // 1. Baseline k=0
    gBenchList.push_back(BaselineIso);

    // 2. Presets (if > 0)
    for (int p = 0; p < 3; ++p) {
        int pk = IsoPresets[p];
        if (pk > 0 && std::find(gBenchList.begin(), gBenchList.end(), pk) == gBenchList.end()) {
            gBenchList.push_back(pk);
        }
    }

    // 3. Regular sweep: 16, 32, ..., 240
    for (int k = SweepStep; k <= 240; k += SweepStep) {
        if (std::find(gBenchList.begin(), gBenchList.end(), k) == gBenchList.end()) {
            gBenchList.push_back(k);
        }
    }

    gBenchIdx = 0;
    gBenchFrame = 0;
    gBenchAccumMs = 0.0f;
    gBenchRunning = true;
    std::cout << "[Bench] Starting benchmark with " << gBenchList.size() << " isovalue steps ("
              << BenchmarkWarmupFrames << " warmup + " << BenchmarkFrames << " sampled frames per step)...\n";
}

static void CancelBenchmark()
{
    if (gBenchRunning) {
        gBenchRunning = false;
        gIsoValue = gSavedIso;
        std::cout << "[Bench] Benchmark cancelled by user.\n";
    }
}

static void FinishBenchmark()
{
    gBenchRunning = false;
    gIsoValue = gSavedIso;

    std::vector<float> regX; // Megatriangles
    std::vector<float> regY; // GPU ms
    gBaselineMs = 0.0f;

    gPlotSweepK.clear();
    gPlotSweepCells.clear();
    gPlotSweepTris.clear();
    gPlotSweepGpuMs.clear();
    gPlotMaxCells = 1.0f;
    gPlotMaxTris = 1.0f;
    gPlotMaxGpuMs = 1.0f;

    for (const auto& r : gBenchResults) {
        if (r.isBaseline) {
            gBaselineMs = r.avgMs;
        } else {
            regX.push_back(static_cast<float>(r.trisGpu * 1e-6));
            regY.push_back(r.avgMs);
        }

        if (r.isSweep) {
            gPlotSweepK.push_back(static_cast<float>(r.k));
            gPlotSweepCells.push_back(static_cast<float>(r.cells));
            gPlotSweepTris.push_back(static_cast<float>(r.trisGpu));
            gPlotSweepGpuMs.push_back(r.avgMs);

            if (r.cells > gPlotMaxCells) gPlotMaxCells = static_cast<float>(r.cells);
            if (r.trisGpu > gPlotMaxTris) gPlotMaxTris = static_cast<float>(r.trisGpu);
            if (r.avgMs > gPlotMaxGpuMs) gPlotMaxGpuMs = r.avgMs;
        }
    }

    FitLine(regX, regY, gFitA, gFitB, gFitR2);

    // Print formatted benchmark results to console with [Bench] tag (Blueprint 9.1)
    std::cout << "\n========================================================================================\n";
    std::cout << "[Bench] COMPLETE BENCHMARK RESULTS TABLE\n";
    std::cout << "========================================================================================\n";
    std::printf("%-5s | %-10s | %-12s | %-12s | %-9s | %-11s | %-10s | %-10s\n",
                "k", "Cells", "Tris(GPU)", "Tris(CPU)", "Tris/Cell", "GPU Time(ms)", "Throughput", "Type");
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& r : gBenchResults) {
        const char* typeStr = r.isBaseline ? "Baseline" : (r.isPreset ? "Preset" : "Sweep");
        std::printf("%-5d | %-10lld | %-12llu | %-12lld | %-9.2f | %-11.3f | %-8.2f M/s | %-10s\n",
                    r.k, r.cells, static_cast<unsigned long long>(r.trisGpu), r.trisCpu,
                    r.trisPerCell, r.avgMs, r.mtrisPerSec, typeStr);
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::printf("[Bench] Linear Fit: GPU ms = %.3f ms + %.3f * (Mtris)   [R^2 = %.4f]\n", gFitA, gFitB, gFitR2);
    std::printf("[Bench] Baseline fixed overhead (k=0): %.3f ms\n", gBaselineMs);
    std::cout << "========================================================================================\n\n";
}

// ── ImGui Setup & Panel ──────────────────────────────────────────────────────

static void InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Do not create an imgui.ini file
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 330");
}

static void ShutdownImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

static void DrawPanel()
{
    ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - PanelWidth - PanelMargin, PanelMargin), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, 0), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Isosurface Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Assignment 2 - Task 2: Data-Driven Isovalue Selection");
        ImGui::Separator();
        ImGui::Text("Performance: %.1f FPS (%.2f ms/frame)",
                    ImGui::GetIO().Framerate,
                    1000.0f / (ImGui::GetIO().Framerate > 0 ? ImGui::GetIO().Framerate : 1.0f));
        ImGui::Text("Volume: %dx%dx%d (%d cells)",
                    gVolume.nx, gVolume.ny, gVolume.nz,
                    std::max(0, gVolume.nx - 1) * std::max(0, gVolume.ny - 1) * std::max(0, gVolume.nz - 1));
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Histogram & Selection (Task 2.1)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox("Log scale", &HistogramLogScale)) {
                RebuildHistPlot();
            }

            const char* overlayText = HistogramLogScale
                ? "cells containing isosurface (log10)"
                : "cells containing isosurface (linear)";

            ImGui::PlotHistogram("##hist", gHistPlot, 256, 0, overlayText, 0.0f, gHistPlotMax,
                                 ImVec2(HistogramPlotWidth, HistogramPlotHeight));

            ImVec2 plotMin = ImGui::GetItemRectMin();
            ImVec2 plotMax = ImGui::GetItemRectMax();
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 innerMin(plotMin.x + style.FramePadding.x, plotMin.y + style.FramePadding.y);
            ImVec2 innerMax(plotMax.x - style.FramePadding.x, plotMax.y - style.FramePadding.y);
            float innerWidth = innerMax.x - innerMin.x;

            auto KToX = [&](float k) -> float {
                return innerMin.x + (k + 0.5f) / 256.0f * innerWidth;
            };

            // Hover tooltip with exact integer cell count and mouse scrubbing
            if (ImGui::IsItemHovered()) {
                ImVec2 mouse = ImGui::GetMousePos();
                float normX = (mouse.x - innerMin.x) / (innerWidth > 0.0f ? innerWidth : 1.0f);
                int kHover = std::clamp(static_cast<int>(std::floor(normX * 256.0f)), IsoMin, IsoMax);
                ImGui::SetTooltip("k = %d   cells = %lld", kHover, gHist[kHover]);
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    gIsoValue = kHover;
                }
            }

            // Draw overlay markers on the plot
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(plotMin, plotMax, true);

            // 1. Detected peaks (triangles at top of histogram plot)
            for (int pk : gDetectedPeaks) {
                float px = KToX(static_cast<float>(pk));
                drawList->AddTriangleFilled(
                    ImVec2(px - 4.0f, innerMin.y),
                    ImVec2(px + 4.0f, innerMin.y),
                    ImVec2(px, innerMin.y + 7.0f),
                    IM_COL32(255, 180, 50, 230));
            }

            // 2. Three presets (distinct colored vertical lines)
            const ImU32 presetColors[3] = {
                IM_COL32(60, 220, 60, 200),   // Preset 1: Green
                IM_COL32(50, 200, 255, 200),  // Preset 2: Cyan
                IM_COL32(255, 100, 220, 200)  // Preset 3: Magenta
            };
            for (int pIdx = 0; pIdx < 3; ++pIdx) {
                float px = KToX(static_cast<float>(IsoPresets[pIdx]));
                drawList->AddLine(ImVec2(px, innerMin.y), ImVec2(px, innerMax.y), presetColors[pIdx], 1.5f);
            }

            // 3. Current active isovalue (bold yellow vertical line)
            float curX = KToX(static_cast<float>(gIsoValue));
            drawList->AddLine(ImVec2(curX, innerMin.y), ImVec2(curX, innerMax.y), IM_COL32(255, 255, 0, 255), 2.5f);

            drawList->PopClipRect();

            // Axis ticks and labels below the plot
            ImGui::Dummy(ImVec2(HistogramPlotWidth, 18.0f));
            ImVec2 tickMin = ImGui::GetItemRectMin();
            const int tickVals[] = {0, 64, 128, 192, 255};
            char tickBuf[16];
            for (int tv : tickVals) {
                float tx = KToX(static_cast<float>(tv));
                drawList->AddLine(ImVec2(tx, tickMin.y), ImVec2(tx, tickMin.y + 4.0f), IM_COL32(200, 200, 200, 255));
                std::snprintf(tickBuf, sizeof(tickBuf), "%d", tv);
                ImVec2 textSz = ImGui::CalcTextSize(tickBuf);
                drawList->AddText(ImVec2(tx - textSz.x * 0.5f, tickMin.y + 5.0f), IM_COL32(200, 200, 200, 255), tickBuf);
            }

            ImGui::Text("x: scalar value k   y: cells containing isosurface   (max = %lld at k = %d)", gHistMaxCells, gHistMaxK);

            // Peak info and quick preset buttons
            if (!gDetectedPeaks.empty()) {
                ImGui::Text("Detected Peaks: ");
                for (std::size_t i = 0; i < gDetectedPeaks.size(); ++i) {
                    ImGui::SameLine();
                    ImGui::Text("[%d]: k=%d (%lld)", static_cast<int>(i + 1), gDetectedPeaks[i], gHist[gDetectedPeaks[i]]);
                }
            }

            // ImGui::Text("Presets: ");
            // ImGui::SameLine();
            // if (ImGui::Button(("1: Peak 1 (" + std::to_string(IsoPresets[0]) + ")").c_str())) gIsoValue = IsoPresets[0];
            // ImGui::SameLine();
            // if (ImGui::Button(("2: Peak 2 (" + std::to_string(IsoPresets[1]) + ")").c_str())) gIsoValue = IsoPresets[1];
            // ImGui::SameLine();
            // if (ImGui::Button(("3: Contrast (" + std::to_string(IsoPresets[2]) + ")").c_str())) gIsoValue = IsoPresets[2];
        }

        if (ImGui::CollapsingHeader("GPU Marching Cubes (Task 2.2)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Isovalue k", &gIsoValue, IsoMin, IsoMax);
            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left) && gIsoValue > IsoMin) gIsoValue--;
            ImGui::SameLine();
            if (ImGui::ArrowButton("##right", ImGuiDir_Right) && gIsoValue < IsoMax) gIsoValue++;
            ImGui::PopButtonRepeat();

            ImGui::Text("Presets: ");
            ImGui::SameLine();
            if (ImGui::Button(("Preset 1 (k=" + std::to_string(IsoPresets[0]) + ")").c_str())) gIsoValue = IsoPresets[0];
            ImGui::SameLine();
            if (ImGui::Button(("Preset 2 (k=" + std::to_string(IsoPresets[1]) + ")").c_str())) gIsoValue = IsoPresets[1];
            ImGui::SameLine();
            if (ImGui::Button(("Preset 3 (k=" + std::to_string(IsoPresets[2]) + ")").c_str())) gIsoValue = IsoPresets[2];

            ImGui::ColorEdit3("Surface Color", SurfaceColor);

            long long activeCells = (gIsoValue >= 1 && gIsoValue <= 255) ? gHist[gIsoValue] : 0LL;
            double trisPerCell = (activeCells > 0) ? static_cast<double>(gLastTris) / activeCells : 0.0;

            ImGui::Separator();
            ImGui::Text("Active Isovalue : %d / 255", gIsoValue);
            ImGui::Text("Active Cells    : %lld / %d (%.1f%%)",
                        activeCells, gNumCells, (gNumCells > 0 ? (activeCells * 100.0 / gNumCells) : 0.0));
            ImGui::Text("Triangles (GPU) : %llu", static_cast<unsigned long long>(gLastTris));
            ImGui::Text("Triangles / Cell: %.2f", trisPerCell);
            ImGui::Text("GPU Draw Time   : %.3f ms", gLastGpuMs);
            ImGui::Text("Volume / Texture: %dx%dx%d -> %dx%dx%d",
                        gVolume.nx, gVolume.ny, gVolume.nz, gTexDimsX, gTexDimsY, gTexDimsZ);

            ImGui::Separator();
            if (ImGui::Button("Check CPU vs GPU")) {
                DrawIso(gIsoValue, true); // Immediate blocking query
                IsoCount cpuCount = CountIso(gVolume, gIsoValue);
                if (gTexDimsX != gVolume.nx || gTexDimsY != gVolume.ny || gTexDimsZ != gVolume.nz) {
                    gCpuGpuCheckResult = "Note: Volume was subsampled for GPU texture; exact count may differ.";
                    gCpuGpuCheckColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                } else if (gLastTris == static_cast<GLuint64>(cpuCount.triangles)) {
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "[PASS] k=%d: GPU=%llu == CPU=%lld triangles (cells=%lld)",
                                  gIsoValue, static_cast<unsigned long long>(gLastTris), cpuCount.triangles, cpuCount.cells);
                    gCpuGpuCheckResult = buf;
                    gCpuGpuCheckColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
                    std::cout << "[Verify] " << buf << '\n';
                } else {
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "[FAIL] k=%d: GPU=%llu != CPU=%lld triangles (diff=%lld)",
                                  gIsoValue, static_cast<unsigned long long>(gLastTris), cpuCount.triangles,
                                  static_cast<long long>(gLastTris) - cpuCount.triangles);
                    gCpuGpuCheckResult = buf;
                    gCpuGpuCheckColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                    std::cout << "[Verify] " << buf << '\n';
                }
            }
            ImGui::TextColored(gCpuGpuCheckColor, "%s", gCpuGpuCheckResult.c_str());
        }

        if (ImGui::CollapsingHeader("Benchmark & Analysis (Task 2.3)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!gBenchRunning) {
                if (ImGui::Button("Run Benchmark")) {
                    StartBenchmark();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(Sweeps baseline k=0, presets, and k=16..240)");
            } else {
                if (ImGui::Button("Cancel")) {
                    CancelBenchmark();
                }
                ImGui::SameLine();
                float progress = (!gBenchList.empty()) ? static_cast<float>(gBenchIdx) / gBenchList.size() : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(220.0f, 0.0f));
                ImGui::SameLine();
                ImGui::Text("Step %d / %d (k=%d)", gBenchIdx + 1, static_cast<int>(gBenchList.size()), gIsoValue);
            }

            if (!gBenchResults.empty()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f),
                                   "Linear Fit: GPU ms = %.3f + %.3f * (Mtris)   [R^2 = %.4f]",
                                   gFitA, gFitB, gFitR2);
                ImGui::Text("Baseline fixed cost (k=0): %.3f ms (cell traversal overhead)", gBaselineMs);

                // 3 Stacked Line Plots over the Sweep Rows (k = 16..240)
                if (gPlotSweepK.size() >= 2) {
                    ImGui::Separator();
                    ImGui::Text("Quantitative Relationships (Sweep k = 16 .. 240)");

                    char overlayCells[64];
                    std::snprintf(overlayCells, sizeof(overlayCells), "Cells (max: %.0f)", gPlotMaxCells);
                    ImGui::PlotLines("##plot_cells", gPlotSweepCells.data(), static_cast<int>(gPlotSweepCells.size()),
                                     0, overlayCells, 0.0f, gPlotMaxCells, ImVec2(HistogramPlotWidth, 55.0f));

                    char overlayTris[64];
                    std::snprintf(overlayTris, sizeof(overlayTris), "Triangles (max: %.0f)", gPlotMaxTris);
                    ImGui::PlotLines("##plot_tris", gPlotSweepTris.data(), static_cast<int>(gPlotSweepTris.size()),
                                     0, overlayTris, 0.0f, gPlotMaxTris, ImVec2(HistogramPlotWidth, 55.0f));

                    char overlayMs[64];
                    std::snprintf(overlayMs, sizeof(overlayMs), "GPU Time (max: %.2f ms)", gPlotMaxGpuMs);
                    ImGui::PlotLines("##plot_ms", gPlotSweepGpuMs.data(), static_cast<int>(gPlotSweepGpuMs.size()),
                                     0, overlayMs, 0.0f, gPlotMaxGpuMs, ImVec2(HistogramPlotWidth, 55.0f));

                    // Shared X axis labels
                    ImGui::Dummy(ImVec2(HistogramPlotWidth, 18.0f));
                    ImVec2 tickMin = ImGui::GetItemRectMin();
                    ImVec2 tickMax = ImGui::GetItemRectMax();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    const ImGuiStyle& style = ImGui::GetStyle();
                    float innerWidth = (tickMax.x - tickMin.x) - 2 * style.FramePadding.x;
                    float startX = tickMin.x + style.FramePadding.x;
                    const int sweepTicks[] = {16, 48, 80, 112, 144, 176, 208, 240};
                    char tBuf[16];
                    for (int tv : sweepTicks) {
                        float norm = static_cast<float>(tv - 16) / (240.0f - 16.0f);
                        float tx = startX + norm * innerWidth;
                        drawList->AddLine(ImVec2(tx, tickMin.y), ImVec2(tx, tickMin.y + 4.0f), IM_COL32(200, 200, 200, 255));
                        std::snprintf(tBuf, sizeof(tBuf), "%d", tv);
                        ImVec2 textSz = ImGui::CalcTextSize(tBuf);
                        drawList->AddText(ImVec2(tx - textSz.x * 0.5f, tickMin.y + 5.0f), IM_COL32(200, 200, 200, 255), tBuf);
                    }
                    ImGui::Text("Notice: Cells, Triangles, and GPU ms follow identical peaks across k!");
                }

                // Results Table
                ImGui::Separator();
                ImGui::Text("Benchmark Results Table (%d entries):", static_cast<int>(gBenchResults.size()));
                if (ImGui::BeginTable("BenchTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 180.0f))) {
                    ImGui::TableSetupColumn("k", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableSetupColumn("Cells", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Tris (GPU)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Tris (CPU)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Tris/Cell", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableSetupColumn("GPU ms", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableSetupColumn("Mtri/s", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableHeadersRow();

                    for (const auto& r : gBenchResults) {
                        ImGui::TableNextRow();
                        if (r.isBaseline) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(30, 60, 90, 160));
                        } else if (r.isPreset) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(60, 80, 40, 160));
                        }

                        ImGui::TableSetColumnIndex(0);
                        if (r.isBaseline) ImGui::Text("%d (Base)", r.k);
                        else if (r.isPreset) ImGui::Text("%d (Pre)", r.k);
                        else ImGui::Text("%d", r.k);

                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", r.cells);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%llu", static_cast<unsigned long long>(r.trisGpu));
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", r.trisCpu);
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f", r.trisPerCell);
                        ImGui::TableSetColumnIndex(5); ImGui::Text("%.3f", r.avgMs);
                        ImGui::TableSetColumnIndex(6); ImGui::Text("%.2f", r.mtrisPerSec);
                    }
                    ImGui::EndTable();
                }
            }
        }
    }
    ImGui::End();
}

// ── Rendering & GLUT Callbacks ───────────────────────────────────────────────

static void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Prepare ImGui frame
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(glutGet(GLUT_WINDOW_WIDTH)),
                            static_cast<float>(glutGet(GLUT_WINDOW_HEIGHT)));
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    static int lastMs = 0;
    float dt = (lastMs > 0) ? (nowMs - lastMs) * 0.001f : (1.0f / 60.0f);
    if (dt <= 0.0f) dt = 1.0f / 60.0f;
    lastMs = nowMs;
    io.DeltaTime = dt;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 2. Draw ImGui UI panel
    DrawPanel();

    // 3. Step Benchmark State Machine (if running) or draw current isosurface
    if (gBenchRunning && gBenchIdx < static_cast<int>(gBenchList.size())) {
        int currentK = gBenchList[gBenchIdx];
        gIsoValue = currentK;
        DrawIso(currentK, true); // Immediate blocking draw for accurate benchmark sampling

        if (gBenchFrame >= BenchmarkWarmupFrames) {
            gBenchAccumMs += gLastGpuMs;
        }
        gBenchFrame++;

        if (gBenchFrame >= BenchmarkWarmupFrames + BenchmarkFrames) {
            float avgMs = (BenchmarkFrames > 0) ? (gBenchAccumMs / BenchmarkFrames) : gLastGpuMs;
            long long cells = (currentK >= 1 && currentK <= 255) ? gHist[currentK] : 0LL;
            IsoCount cpuCount = CountIso(gVolume, currentK);
            double trisPerCell = (cells > 0) ? static_cast<double>(gLastTris) / cells : 0.0;
            double mtrisPerSec = (avgMs > 0.0001f) ? (gLastTris * 1e-3 / avgMs) : 0.0;

            bool isBaseline = (currentK == BaselineIso);
            bool isPreset = (currentK == IsoPresets[0] || currentK == IsoPresets[1] || currentK == IsoPresets[2]);
            bool isSweep = (!isBaseline && currentK % SweepStep == 0);

            BenchRow row;
            row.k = currentK;
            row.cells = cells;
            row.trisGpu = gLastTris;
            row.trisCpu = cpuCount.triangles;
            row.trisPerCell = trisPerCell;
            row.avgMs = avgMs;
            row.mtrisPerSec = mtrisPerSec;
            row.isBaseline = isBaseline;
            row.isPreset = isPreset;
            row.isSweep = isSweep;
            gBenchResults.push_back(row);

            gBenchFrame = 0;
            gBenchAccumMs = 0.0f;
            gBenchIdx++;

            if (gBenchIdx >= static_cast<int>(gBenchList.size())) {
                FinishBenchmark();
            }
        }
    } else {
        // Normal interactive rendering
        DrawIso(gIsoValue, false);
    }

    // 4. Render ImGui over 3D scene
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    CheckGLError("RenderSceneCB");
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

static void IdleCB()
{
    glutPostRedisplay();
}

static void KeyboardCB(unsigned char key, int mouse_x, int mouse_y)
{
    switch (key) {
        case '1':
            ModelWorld.Rotate(ModelRotationStepDeg, 0.0f, 0.0f);
            break;
        case '2':
            ModelWorld.Rotate(-ModelRotationStepDeg, 0.0f, 0.0f);
            break;
        case '3':
            ModelWorld.Rotate(0.0f, ModelRotationStepDeg, 0.0f);
            break;
        case '4':
            ModelWorld.Rotate(0.0f, -ModelRotationStepDeg, 0.0f);
            break;
        case '5':
            ModelWorld.Rotate(0.0f, 0.0f, ModelRotationStepDeg);
            break;
        case '6':
            ModelWorld.Rotate(0.0f, 0.0f, -ModelRotationStepDeg);
            break;
        case '0':
            ModelWorld.ResetTransform();
            break;

        case 'p':
        case 'P':
            ModelProject.ToggleProjection();
            break;

        case 27: // ESC
            ShutdownImGui();
            glutDestroyWindow(glutGetWindow());
            std::exit(0);

        default:
            ModelView.OnKeyboard(key);
            break;
    }

    glutPostRedisplay();
}

static void SpecialKeyboardCB(int key, int mouse_x, int mouse_y)
{
    switch (key) {
        case GLUT_KEY_LEFT:
            ModelWorld.Translate(-ModelTranslateStep, 0.0f, 0.0f);
            break;
        case GLUT_KEY_RIGHT:
            ModelWorld.Translate(ModelTranslateStep, 0.0f, 0.0f);
            break;
        case GLUT_KEY_UP:
            ModelWorld.Translate(0.0f, ModelTranslateStep, 0.0f);
            break;
        case GLUT_KEY_DOWN:
            ModelWorld.Translate(0.0f, -ModelTranslateStep, 0.0f);
            break;
        case GLUT_KEY_F1:
            IsFullScreen = !IsFullScreen;
            if (IsFullScreen) {
                WindowPositionX = glutGet(GLUT_WINDOW_X);
                WindowPositionY = glutGet(GLUT_WINDOW_Y);
                glutFullScreen();
            } else {
                glutReshapeWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT));
                glutPositionWindow(WindowPositionX, WindowPositionY);
            }
            break;
        default:
            break;
    }

    glutPostRedisplay();
}

// ── Mouse-only FreeGLUT Glue for ImGui ────────────────────────────────────────

static void MouseMotionCB(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));
    glutPostRedisplay();
}

static void MouseCB(int button, int state, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));

    int imguiButton = 0;
    if (button == GLUT_LEFT_BUTTON) imguiButton = 0;
    else if (button == GLUT_RIGHT_BUTTON) imguiButton = 1;
    else if (button == GLUT_MIDDLE_BUTTON) imguiButton = 2;

    io.AddMouseButtonEvent(imguiButton, state == GLUT_DOWN);
    glutPostRedisplay();
}

static void PrintControls()
{
    std::cout << "\n======================================================================\n";
    std::cout << " Task 2: Data-Driven Isovalue Selection & GPU Marching Cubes\n";
    std::cout << "======================================================================\n";
    std::cout << " CLI Invocation Options:\n";
    std::cout << "   ./main                                : Load default bonsai dataset (256^3)\n";
    std::cout << "   ./main <path.raw> [nx ny nz]          : Load raw volume (uint8 or uint16)\n";
    std::cout << "   ./main --synthetic ramp|sphere|corner : Load synthetic test dataset\n";
    std::cout << "   ./main --test                         : Run complete pre-flight test suite\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << " ImGui Control Panel:\n";
    std::cout << "   Histogram Section : Log/Linear scale, hover cell counts, scrub k live\n";
    std::cout << "   Isosurface Section: Isovalue slider, +/- buttons, 3 presets, color picker\n";
    std::cout << "   CPU vs GPU Button : Hardware GL_PRIMITIVES_GENERATED vs CPU CountIso\n";
    std::cout << "   Benchmark Section : Non-blocking sweep, stacked plots, linear regression\n";
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << " Keyboard Controls (FreeGLUT):\n";
    std::cout << "   1 / 2       : Rotate model around X axis (+ / -)\n";
    std::cout << "   3 / 4       : Rotate model around Y axis (+ / -)\n";
    std::cout << "   5 / 6       : Rotate model around Z axis (+ / -)\n";
    std::cout << "   0           : Reset model rotation and translation\n";
    std::cout << "   Arrow Keys  : Translate model in X / Y plane\n";
    std::cout << "   i / k       : Camera move forward / backward\n";
    std::cout << "   j / l       : Camera move left / right\n";
    std::cout << "   u / o       : Camera move up / down\n";
    std::cout << "   p / P       : Toggle perspective / orthographic projection\n";
    std::cout << "   F1          : Toggle fullscreen mode\n";
    std::cout << "   ESC         : Clean shutdown and exit\n";
    std::cout << "======================================================================\n\n";
}

// ── Pre-flight Verification Suite ───────────────────────────────────────────

static bool RunPreflightVerifications()
{
    std::cout << "[Verify] Task 2.1 pre-flight verification test suite.\n";

    // 1. Raw byte copy check (7.1)
    Volume testVol;
    testVol.CreateSynthetic("sphere", 16, 16, 16);
    std::vector<unsigned char> raw = GetRawVolumeBytes(testVol);
    if (raw.size() != 16 * 16 * 16) {
        std::cerr << "  [FAIL] Raw byte size mismatch.\n";
        return false;
    }
    int checkIndices[] = {0, 16 * 16 * 16 - 1, 123, 500, 2048};
    for (int idx : checkIndices) {
        int z = idx / (16 * 16);
        int rem = idx % (16 * 16);
        int y = rem / 16;
        int x = rem % 16;
        unsigned char expected = static_cast<unsigned char>(std::round(testVol.At(x, y, z) * 255.0f));
        if (raw[idx] != expected) {
            std::cerr << "  [FAIL] Raw byte mismatch at (" << x << "," << y << "," << z << "): got "
                      << static_cast<int>(raw[idx]) << ", expected " << static_cast<int>(expected) << '\n';
            return false;
        }
    }
    std::cout << "  [PASS] Raw volume byte copy verified.\n";

    // 2. Synthetic Corner check (7.7a): exactly 685 cells for all k in 1..255
    Volume cornerVol;
    cornerVol.CreateSynthetic("corner", 64, 48, 32);
    std::vector<long long> cornerHist = BuildIsoHistogram(cornerVol);
    if (cornerHist.size() != 256) {
        std::cerr << "  [FAIL] Corner histogram size != 256.\n";
        return false;
    }
    for (int k = 1; k <= 255; ++k) {
        if (cornerHist[k] != 685) {
            std::cerr << "  [FAIL] Corner hist[" << k << "] = " << cornerHist[k] << " (expected 685)\n";
            return false;
        }
    }
    std::cout << "  [PASS] Synthetic corner: exactly 685 cells for all k in [1, 255].\n";

    // 3. Synthetic Ramp check (7.7b): exactly 1457 cells ((48-1)*(32-1) = 47*31 = 1457) for all k in 1..255
    Volume rampVol;
    rampVol.CreateSynthetic("ramp", 64, 48, 32);
    std::vector<long long> rampHist = BuildIsoHistogram(rampVol);
    for (int k = 1; k <= 255; ++k) {
        if (rampHist[k] != 1457) {
            std::cerr << "  [FAIL] Ramp hist[" << k << "] = " << rampHist[k] << " (expected 1457)\n";
            return false;
        }
    }
    std::cout << "  [PASS] Synthetic ramp: exactly 1457 cells for all k in [1, 255].\n";

    // 4. Sum check (7.7c): sum_{k=1..255} hist[k] == sum_{cells} (M - m)
    long long histSum = 0;
    for (int k = 1; k <= 255; ++k) histSum += cornerHist[k];
    std::vector<unsigned char> cRaw = GetRawVolumeBytes(cornerVol);
    long long cellRangeSum = 0;
    for (int z = 0; z < cornerVol.nz - 1; ++z) {
        for (int y = 0; y < cornerVol.ny - 1; ++y) {
            for (int x = 0; x < cornerVol.nx - 1; ++x) {
                unsigned char c0 = cRaw[x + 64 * (y + 48 * z)];
                unsigned char c1 = cRaw[(x + 1) + 64 * (y + 48 * z)];
                unsigned char c2 = cRaw[(x + 1) + 64 * ((y + 1) + 48 * z)];
                unsigned char c3 = cRaw[x + 64 * ((y + 1) + 48 * z)];
                unsigned char c4 = cRaw[x + 64 * (y + 48 * (z + 1))];
                unsigned char c5 = cRaw[(x + 1) + 64 * (y + 48 * (z + 1))];
                unsigned char c6 = cRaw[(x + 1) + 64 * ((y + 1) + 48 * (z + 1))];
                unsigned char c7 = cRaw[x + 64 * ((y + 1) + 48 * (z + 1))];
                unsigned char m = std::min({c0, c1, c2, c3, c4, c5, c6, c7});
                unsigned char M = std::max({c0, c1, c2, c3, c4, c5, c6, c7});
                cellRangeSum += (M - m);
            }
        }
    }
    if (histSum != cellRangeSum) {
        std::cerr << "  [FAIL] Sum check failed: histSum=" << histSum << ", cellRangeSum=" << cellRangeSum << '\n';
        return false;
    }
    std::cout << "  [PASS] Sum check: sum(hist[k]) == sum(M - m) = " << histSum << ".\n";

    // 5. CountActiveCells equality check (7.7d) for k = 1, 50, 128, 255
    int testK[] = {1, 50, 128, 255};
    for (int k : testK) {
        long long direct = CountActiveCells(rampVol, k);
        if (direct != rampHist[k]) {
            std::cerr << "  [FAIL] CountActiveCells mismatch for k=" << k << ": " << direct << " vs " << rampHist[k] << '\n';
            return false;
        }
    }
    std::cout << "  [PASS] CountActiveCells matches BuildIsoHistogram for sample k values.\n";

    // 6. Peak finder check (7.5)
    std::vector<long long> synthHist(256, 0);
    synthHist[40] = 100;
    synthHist[39] = 70;  synthHist[41] = 70;
    synthHist[45] = 90;
    synthHist[44] = 60;  synthHist[46] = 60;
    synthHist[200] = 80;
    synthHist[199] = 50; synthHist[201] = 50;
    std::vector<int> peaks = FindPeaks(synthHist, 1, 16, 5);
    if (peaks.size() != 2 || peaks[0] != 40 || peaks[1] != 200) {
        std::cerr << "  [FAIL] Peak finder failed: returned ";
        for (int p : peaks) std::cerr << p << " ";
        std::cerr << " (expected 40, 200)\n";
        return false;
    }
    std::cout << "  [PASS] Peak finder correctly identified peaks and enforced separation.\n";

    // ── Task 2.2 Pre-flight Verifications ──
    // 7. TriTable Structural Verification (Blueprint 8.1)
    if (GetTriCount(0) != 0 || GetTriCount(255) != 0) {
        std::cerr << "  [FAIL] TriTable rows 0 or 255 not empty.\n";
        return false;
    }
    for (int c = 0; c < 256; ++c) {
        int count = 0;
        while (count < 16 && TriTable[c][count] != -1) count++;
        if (count % 3 != 0 || count / 3 > 5) {
            std::cerr << "  [FAIL] TriTable row " << c << " invalid length: " << count << '\n';
            return false;
        }

        // Property (c): Set of active edges in TriTable[c] == edges with differing corner bits
        std::vector<int> rowEdges;
        for (int i = 0; i < count; ++i) rowEdges.push_back(TriTable[c][i]);
        std::sort(rowEdges.begin(), rowEdges.end());
        rowEdges.erase(std::unique(rowEdges.begin(), rowEdges.end()), rowEdges.end());

        std::vector<int> expectedEdges;
        for (int e = 0; e < 12; ++e) {
            int bitA = (c >> EdgeCorner[e][0]) & 1;
            int bitB = (c >> EdgeCorner[e][1]) & 1;
            if (bitA != bitB) expectedEdges.push_back(e);
        }
        std::sort(expectedEdges.begin(), expectedEdges.end());

        if (rowEdges != expectedEdges) {
            std::cerr << "  [FAIL] TriTable row " << c << " edge set mismatch.\n";
            return false;
        }
    }
    std::cout << "  [PASS] TriTable: all 256 configurations topologically sound and edge-parity verified.\n";

    // 8. CPU Marching Cubes Counter Verification (Blueprint 7.6 / 8.12)
    IsoCount cornerCount = CountIso(cornerVol, 128);
    if (cornerCount.cells != 685 || cornerCount.triangles <= 0) {
        std::cerr << "  [FAIL] CountIso cornerVol mismatch: cells=" << cornerCount.cells
                  << ", triangles=" << cornerCount.triangles << '\n';
        return false;
    }
    IsoCount rampCount = CountIso(rampVol, 128);
    if (rampCount.cells != 1457 || rampCount.triangles <= 0) {
        std::cerr << "  [FAIL] CountIso rampVol mismatch: cells=" << rampCount.cells
                  << ", triangles=" << rampCount.triangles << '\n';
        return false;
    }
    if (CountIso(cornerVol, 0).triangles != 0) {
        std::cerr << "  [FAIL] CountIso baseline check (k=0) produced non-zero triangles.\n";
        return false;
    }
    std::cout << "  [PASS] CPU CountIso matches exact cell counts and baseline k=0 produces 0 triangles.\n";

    // 9. Linear Regression Fit Verification (Blueprint 9.3)
    std::vector<float> testX = {1.0f, 2.0f, 3.0f};
    std::vector<float> testY = {2.0f, 4.0f, 6.0f};
    float testA = 0.0f, testB = 0.0f, testR2 = 0.0f;
    if (!FitLine(testX, testY, testA, testB, testR2)) {
        std::cerr << "  [FAIL] FitLine returned false.\n";
        return false;
    }
    if (std::abs(testA - 0.0f) > 1e-4f || std::abs(testB - 2.0f) > 1e-4f || std::abs(testR2 - 1.0f) > 1e-4f) {
        std::cerr << "  [FAIL] FitLine mismatch: a=" << testA << ", b=" << testB << ", R2=" << testR2 << '\n';
        return false;
    }
    std::cout << "  [PASS] FitLine mathematical verification passed (a=0, b=2, R^2=1.0).\n";

    std::cout << "[Verify] ALL Task 2.1, 2.2 & 2.3 pre-flight verification tests PASSED!\n";
    return true;
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    if (argc >= 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
        PrintControls();
        return 0;
    }

    if (argc >= 2 && std::strcmp(argv[1], "--test") == 0) {
        return RunPreflightVerifications() ? 0 : 1;
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
            for (std::size_t i = 0; i < path.size(); ++i) {
                if (std::sscanf(path.c_str() + i, "%dx%dx%d", &nx, &ny, &nz) == 3) {
                    break;
                }
            }
            if (nx <= 0 || ny <= 0 || nz <= 0) {
                nx = DefaultDimsX; ny = DefaultDimsY; nz = DefaultDimsZ;
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
        std::cout << "Defaulting to " << DefaultDataPath << " " << DefaultDimsX << " " << DefaultDimsY << " " << DefaultDimsZ << "\n";
        if (!gVolume.LoadRaw(DefaultDataPath, DefaultDimsX, DefaultDimsY, DefaultDimsZ)) {
            std::cout << "Fallback to synthetic sphere (64x48x32)\n";
            gVolume.CreateSynthetic("sphere", 64, 48, 32);
        }
    }

    // Compute isovalue cell histogram (Task 2.1)
    auto tStart = std::chrono::high_resolution_clock::now();
    gHist = BuildIsoHistogram(gVolume);
    auto tEnd = std::chrono::high_resolution_clock::now();
    double histMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "[Hist] done in " << histMs << " ms\n";

    gDetectedPeaks = FindPeaks(gHist, PeakSmoothRadius, MinPeakSeparation, PeakCount);
    std::cout << "[Hist] Detected " << gDetectedPeaks.size() << " peaks: ";
    for (std::size_t i = 0; i < gDetectedPeaks.size(); ++i) {
        std::cout << "k=" << gDetectedPeaks[i] << " (" << gHist[gDetectedPeaks[i]] << " cells)"
                  << (i + 1 < gDetectedPeaks.size() ? ", " : "\n");
    }

    // Find global max bin
    gHistMaxK = 1;
    gHistMaxCells = gHist[1];
    for (int k = 1; k <= 255; ++k) {
        if (gHist[k] > gHistMaxCells) {
            gHistMaxCells = gHist[k];
            gHistMaxK = k;
        }
    }

    // Preset selection rule (Blueprint 7.5):
    // Select the two dominant peaks (>= 16 apart) and sort them in ascending scalar
    // density order (k1 < k2) so Presets 1 -> 2 -> 3 peel naturally from outside to inside.
    if (gDetectedPeaks.size() >= 2) {
        IsoPresets[0] = std::min(gDetectedPeaks[0], gDetectedPeaks[1]);
        IsoPresets[1] = std::max(gDetectedPeaks[0], gDetectedPeaks[1]);
    } else if (!gDetectedPeaks.empty()) {
        IsoPresets[0] = gDetectedPeaks[0];
        IsoPresets[1] = std::min(255, IsoPresets[0] + 32);
    }
    IsoPresets[2] = 160; // Preset 3: Contrast / high-k dense core

    gIsoValue = IsoPresets[0];
    RebuildHistPlot();

    // GLUT setup
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

    std::cout << "[UI] OpenGL Version: " << glGetString(GL_VERSION) << '\n';
    if (!GLEW_VERSION_3_2) {
        std::cerr << "[UI] Error: OpenGL 3.2 or Geometry Shader support is required.\n";
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(BgColorR, BgColorG, BgColorB, BgColorA);

    // Initialize ImGui
    InitImGui();

    // Initialize GPU Marching Cubes Pipeline (Task 2.2)
    InitIsoPipeline();

    // Camera and model setup
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

    // Register GLUT callbacks
    glutDisplayFunc(RenderSceneCB);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(KeyboardCB);
    glutSpecialFunc(SpecialKeyboardCB);
    glutPassiveMotionFunc(MouseMotionCB);
    glutMotionFunc(MouseMotionCB);
    glutMouseFunc(MouseCB);
    glutIdleFunc(IdleCB);

    PrintControls();

    glutMainLoop();
    return 0;
}
