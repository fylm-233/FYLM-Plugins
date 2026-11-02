#include "retro/retro.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include "embedded_shaders.hpp"
#include "blue_noise.inc"
namespace retro {
// Central named-field mapping prevents GLSL/C++ ABI or bool-padding assumptions.
#define CRT_FLOATS(X)                                                                                        \
    X(curvature)                                                                                             \
    X(scanlineCount) X(scanlineOpacity) X(luminanceBlur) X(chromaBleed) X(chromaShiftPixels)                 \
        X(trackingHeight) X(trackingDistortion) X(trackingNoise) X(wobbleIntensity) X(wobbleFrequency)       \
            X(noiseAmount) X(noiseColor) X(streakFrequency) X(streakIntensity) X(streakHeight)               \
                X(dropoutFrequency) X(dropoutIntensity) X(dropoutHeight) X(maskDensity) X(maskIntensity)     \
                    X(convergence) X(halationIntensity) X(halationRadius) X(brightness) X(contrast)          \
                        X(saturation) X(vignetteIntensity) X(vignetteSmoothness) X(cornerRadius) X(borderX)  \
                            X(borderY) X(borderSmoothness)
bool validate(const Controls &c, std::string &error) {
    auto fail = [&](const char *s) {
        error = s;
        return false;
    };
    if (c.order != Order::DitherThenRetro && c.order != Order::RetroThenDither)
        return fail("retro.error.order");
    if (c.workingResolution < 0 || c.workingResolution > 8192)
        return fail("retro.error.resolution");
    if (!std::isfinite(c.timeRate) || !std::isfinite(c.timeOffset))
        return fail("retro.error.time");
    if (c.dither.enabled && (c.dither.algorithmId != 2 && c.dither.algorithmId != 8 &&
                             c.dither.algorithmId != 15 && c.dither.algorithmId != 17))
        return fail("retro.error.cpu_algorithm");
    if (c.dither.paletteSize < 0 || c.dither.paletteSize > 256 || !std::isfinite(c.dither.threshold) ||
        c.dither.threshold < 0 || c.dither.threshold > 255)
        return fail("retro.error.dither");
#define CHECK_FINITE(name)                                                                                   \
    if (!std::isfinite(c.crt.name))                                                                          \
        return fail("retro.error.nonfinite");
    CRT_FLOATS(CHECK_FINITE)
#undef CHECK_FINITE
    auto unit = [](float f) { return f >= 0 && f <= 1; };
    if (!unit(c.crt.curvature) || !unit(c.crt.scanlineOpacity) || !unit(c.crt.trackingHeight) ||
        !unit(c.crt.trackingDistortion) || !unit(c.crt.trackingNoise) || !unit(c.crt.noiseAmount) ||
        !unit(c.crt.noiseColor) || !unit(c.crt.maskIntensity) || !unit(c.crt.convergence) ||
        !unit(c.crt.halationIntensity) || !unit(c.crt.vignetteIntensity))
        return fail("retro.error.range");
    if (!unit(c.crt.streakFrequency) || !unit(c.crt.streakIntensity) || !unit(c.crt.streakHeight) ||
        !unit(c.crt.dropoutFrequency) || !unit(c.crt.dropoutIntensity) || !unit(c.crt.dropoutHeight))
        return fail("retro.error.range");
    if (c.crt.scanlineCount < 0 || c.crt.scanlineCount > 8192 || c.crt.luminanceBlur < 0 ||
        c.crt.luminanceBlur > 5 || c.crt.chromaBleed < 0 || c.crt.chromaBleed > 10 ||
        std::abs(c.crt.chromaShiftPixels) > 20 || c.crt.wobbleIntensity < 0 || c.crt.wobbleIntensity > .1f ||
        c.crt.wobbleFrequency < 0 || c.crt.wobbleFrequency > 100)
        return fail("retro.error.range");
    if (c.crt.maskDensity < 0 || c.crt.maskDensity > 8192 || c.crt.halationRadius < 0 ||
        c.crt.halationRadius > 5 || std::abs(c.crt.brightness) > 1 || c.crt.contrast < 0 ||
        c.crt.contrast > 3 || c.crt.saturation < 0 || c.crt.saturation > 3 || c.crt.vignetteSmoothness < 0 ||
        c.crt.vignetteSmoothness > 2 || c.crt.cornerRadius < 0 || c.crt.cornerRadius > .5f ||
        c.crt.borderX < 0 || c.crt.borderX > .5f || c.crt.borderY < 0 || c.crt.borderY > .5f ||
        c.crt.borderSmoothness < 0 || c.crt.borderSmoothness > .1f)
        return fail("retro.error.range");
    error.clear();
    return true;
}
UniformFrame makeUniformFrame(const Controls &c, Size size, std::int64_t value, std::uint64_t scale) {
    std::string error;
    if (!validate(c, error))
        throw std::invalid_argument(error);
    if (size.width <= 0 || size.height <= 0 || !scale)
        throw std::invalid_argument("retro.error.frame");
    double t = (c.animateTime ? double(value) / double(scale) * c.timeRate : 0.0) + c.timeOffset;
    if (!std::isfinite(t) || std::abs(t) > std::numeric_limits<float>::max())
        throw std::invalid_argument("retro.error.time");
    return {size, float(t), c.crt, c.dither};
}
Plan makePlan(const Controls &c, Size full) {
    Plan p;
    if (!validate(c, p.error))
        return p;
    if (full.width <= 0 || full.height <= 0) {
        p.error = "retro.error.frame";
        return p;
    }
    double scale = c.workingResolution > 0
                       ? std::min(1.0, double(c.workingResolution) / std::max(full.width, full.height))
                       : 1;
    Size work{std::max(1, int(std::floor(full.width * scale + .5))),
              std::max(1, int(std::floor(full.height * scale + .5)))};
    bool resize = work.width != full.width || work.height != full.height;
    auto dither = [&] {
        if (!c.dither.enabled)
            return;
        if (resize)
            p.stages.push_back({Pass::DownsampleLinear, work});
        p.stages.push_back({Pass::Dither, work});
        if (resize)
            p.stages.push_back({Pass::UpsampleNearest, full});
    };
    auto analog = [&] {
        if (!c.retroEnabled)
            return;
        if (c.crt.vhsEnabled)
            p.stages.push_back({Pass::VHS, full});
        if (c.crt.crtEnabled)
            p.stages.push_back({Pass::CRT, full});
    };
    if (c.order == Order::DitherThenRetro) {
        dither();
        analog();
    } else {
        analog();
        dither();
    }
    p.ok = true;
    return p;
}
const std::uint16_t *blueNoiseRanking() noexcept {
    static_assert(sizeof(unsigned short) == 2);
    return blueNoise;
}
const char *crtShader() noexcept { return embedded::crt; }
const char *ditherShader() noexcept { return embedded::dither; }
const char *vertexShader() noexcept { return embedded::vertex; }
std::string fragmentShader(int variant) {
    if (variant < 0 || variant > 6)
        throw std::invalid_argument("retro.error.variant");
    return std::string("#version 330 core\n#define RETRO_PASS ") + std::to_string(variant) + "\n" +
           embedded::crt + "\n" + embedded::dither + "\n" + embedded::entry;
}
UniformLocations locateUniforms(const UniformApi &api, std::uint32_t program) {
    if (!api.location)
        throw std::invalid_argument("retro.error.uniform_api");
    UniformLocations result;
    auto add = [&](const char *name) {
        result.entries.push_back({api.location(api.context, program, name), name});
    };
    for (auto name :
         {"uInput", "uPalette", "uNoiseRanks", "uResolution", "uWorkOrigin", "uTime", "uCrt.vhsEnabled",
          "uCrt.crtEnabled", "uCrt.clampOutput", "uDither.enabled", "uDither.algorithmId",
          "uDither.paletteSize", "uDither.invert", "uDither.preserveAlpha", "uDither.threshold"})
        add(name);
#define LOCATE(name) add("uCrt." #name);
    CRT_FLOATS(LOCATE)
#undef LOCATE
    return result;
}
void bindUniforms(const UniformApi &api, const UniformLocations &locations, const UniformFrame &f, int ox,
                  int oy) {
    if (!api.oneFloat || !api.oneInt || !api.twoFloat || !api.twoInt)
        throw std::invalid_argument("retro.error.uniform_api");
    for (const auto &entry : locations.entries) {
        if (entry.location < 0)
            continue;
        auto n = std::string_view(entry.name);
        int l = entry.location;
        if (n == "uInput")
            api.oneInt(api.context, l, 0);
        else if (n == "uPalette")
            api.oneInt(api.context, l, 1);
        else if (n == "uNoiseRanks")
            api.oneInt(api.context, l, 2);
        else if (n == "uResolution")
            api.twoFloat(api.context, l, float(f.resolution.width), float(f.resolution.height));
        else if (n == "uWorkOrigin")
            api.twoInt(api.context, l, ox, oy);
        else if (n == "uTime")
            api.oneFloat(api.context, l, f.timeSeconds);
#define BIND_CRT_INT(name) else if (n == "uCrt." #name) api.oneInt(api.context, l, f.crt.name);
        BIND_CRT_INT(vhsEnabled)
        BIND_CRT_INT(crtEnabled) BIND_CRT_INT(clampOutput)
#undef BIND_CRT_INT
#define BIND_FLOAT(name) else if (n == "uCrt." #name) api.oneFloat(api.context, l, f.crt.name);
            CRT_FLOATS(BIND_FLOAT)
#undef BIND_FLOAT
#define BIND_DITHER_INT(name) else if (n == "uDither." #name) api.oneInt(api.context, l, f.dither.name);
                BIND_DITHER_INT(enabled) BIND_DITHER_INT(algorithmId) BIND_DITHER_INT(paletteSize)
                    BIND_DITHER_INT(invert) BIND_DITHER_INT(preserveAlpha)
#undef BIND_DITHER_INT
                        else if (n == "uDither.threshold") api.oneFloat(api.context, l, f.dither.threshold);
    }
}
} // namespace retro

