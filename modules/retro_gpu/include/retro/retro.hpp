#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace retro {
enum class Order { DitherThenRetro = 1, RetroThenDither = 2 };
enum class Pass { DownsampleLinear, Dither, UpsampleNearest, VHS, CRT };
struct Size {
    int width = 0, height = 0;
};
// Named uniforms, NOT a std140/std430 binary layout. Do not memcpy this into a UBO.
struct CRTVHSParams {
    std::int32_t maskType=1;
    float scanlineSharpness=1.5f;
    std::int32_t vhsEnabled = 1, crtEnabled = 1, clampOutput = 1;
    float curvature = 0;
    float scanlineCount = 288, scanlineOpacity = .25f;
    float luminanceBlur = 1.2f, chromaBleed = 4, chromaShiftPixels = 2;
    float trackingHeight = .4f, trackingDistortion = .01f, trackingNoise = .6f;
    float wobbleIntensity = .002f, wobbleFrequency = 4;
    float noiseAmount = .1f, noiseColor = .3f;
    float streakFrequency = .02f, streakIntensity = .1f, streakHeight = .02f;
    float dropoutFrequency = .02f, dropoutIntensity = .2f, dropoutHeight = .2f;
    float maskDensity = 400, maskIntensity = .2f, convergence = .02f;
    float halationIntensity = .5f, halationRadius = 2;
    float brightness = -.1f, contrast = 1.25f, saturation = .75f;
    float vignetteIntensity = .5f, vignetteSmoothness = .5f;
    float cornerRadius = .01f, borderX = .006f, borderY = .008f, borderSmoothness = .002f;
};
struct DitherParams {
    std::int32_t enabled = 1, algorithmId = 2, paletteSize = 0, invert = 0, preserveAlpha = 0;
    float threshold = 128;
};
struct Controls {
    bool retroEnabled = false; // Opt-in: old projects must keep the 0.2 appearance.
    Order order = Order::DitherThenRetro;
    int workingResolution = 512;
    bool animateTime = true;
    double timeRate = 1, timeOffset = 0;
    CRTVHSParams crt;
    DitherParams dither;
    float toneContrast=0,toneMidtones=1,toneHighlights=0;
};
struct UniformFrame {
    Size resolution;
    float timeSeconds = 0;
    CRTVHSParams crt;
    DitherParams dither;
};
struct Stage {
    Pass pass;
    Size size;
};
struct Plan {
    bool ok = false;
    std::string error;
    std::vector<Stage> stages;
};
bool validate(const Controls &controls, std::string &error);
UniformFrame makeUniformFrame(const Controls &controls, Size resolution, std::int64_t timeValue,
                              std::uint64_t timeScale);
Plan makePlan(const Controls &controls, Size full);
const std::uint16_t *blueNoiseRanking() noexcept; // 4096 immutable ranks, upload GL_R16UI, 64x64.
// Immutable embedded sources; no external .glsl files are needed by the eventual .aex.
const char *crtShader() noexcept;
const char *ditherShader() noexcept;
const char *vertexShader() noexcept;
std::string fragmentShader(int variant); // 0(copy),1(dither),2(VHS),3(CRT),4(convenience),5(tone),6(alpha).
struct UniformApi {
    void *context = nullptr;
    int (*location)(void *, std::uint32_t, const char *) = nullptr;
    void (*oneFloat)(void *, int, float) = nullptr;
    void (*oneInt)(void *, int, int) = nullptr;
    void (*twoFloat)(void *, int, float, float) = nullptr;
    void (*twoInt)(void *, int, int, int) = nullptr;
};
// Call once after program link; retain per-context/per-program (never process-global).
struct UniformLocations {
    struct Entry {
        int location;
        std::string name;
    };
    std::vector<Entry> entries;
};
UniformLocations locateUniforms(const UniformApi &, std::uint32_t program);
// Caller binds the matching program and textures. Input=unit0, palette=unit1, ranks=unit2.
void bindUniforms(const UniformApi &, const UniformLocations &, const UniformFrame &, int workOriginX = 0,
                  int workOriginY = 0);
} // namespace retro
