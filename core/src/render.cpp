// Derived from Oslo418's Dithering Studio (Apache-2.0). Native C++ port, 2026.
#include "blue_noise.inc"
#include "dither/core.hpp"
#include "dither/halftone.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace dither {
const char *errorKey(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::None:
        return "";
    case ErrorCode::InvalidImage:
        return "error.invalid_image";
    case ErrorCode::UnsupportedFormat:
        return "error.unsupported_format";
    case ErrorCode::InvalidStride:
        return "error.invalid_stride";
    case ErrorCode::UnsupportedAlgorithm:
        return "error.unsupported_algorithm";
    case ErrorCode::InvalidParams:
        return "error.invalid_params";
    case ErrorCode::Overflow:
        return "error.overflow";
    case ErrorCode::Cancelled:
        return "error.cancelled";
    case ErrorCode::OutOfMemory:
        return "error.memory";
    default:
        return "error.internal";
    }
}
std::uint8_t toU8Clamped(double v) noexcept {
    if (!(v > 0))
        return 0;
    if (v >= 255)
        return 255;
    double f = std::floor(v), d = v - f;
    return static_cast<std::uint8_t>(f + (d > 0.5 || (d == 0.5 && static_cast<int>(f) % 2)));
}
namespace {
struct Failure {
    ErrorCode code;
    const char *message;
};
RenderResult failure(ErrorCode code, const char *message) noexcept {
    RenderResult result{false, {}, code};
    try {
        result.error = message;
    } catch (...) {
    }
    return result;
}
double clamp(double v, double lo = 0, double hi = 255) { return std::max(lo, std::min(hi, v)); }
void tick(IProgress *p, int y, int h) {
    if (p && y % 16 == 0) {
        if (p->cancelled())
            throw Failure{ErrorCode::Cancelled, "cancelled"};
        p->report(double(y) / h);
    }
}
std::uint8_t read(const ConstImageView &s, int x, int y, int c) {
    auto q = static_cast<const std::uint8_t *>(s.data) + y * s.rowBytes;
    if (s.format == PixelFormat::RGBA8)
        return q[x * 4 + c];
    std::uint16_t v;
    std::memcpy(&v, q + (x * 4 + c) * 2, 2);
    return toU8Clamped(v / 257.0);
}
void tone(std::vector<std::uint8_t> &a, ToneParams t) {
    double cf = 1 + (std::isfinite(t.contrast) ? clamp(t.contrast, -100, 100) : 0) / 100;
    double gm = std::isfinite(t.midtones) ? clamp(t.midtones, .5, 2) : 1;
    double ha = (std::isfinite(t.highlights) ? clamp(t.highlights, 0, 100) : 0) / 100;
    if (std::abs(cf - 1) < 1e-3 && std::abs(gm - 1) < 1e-3 && ha < 1e-3)
        return;
    for (size_t i = 0; i < a.size(); i += 4) {
        double L = .299 * a[i] + .587 * a[i + 1] + .114 * a[i + 2];
        double l = std::pow(clamp(.5 + (L / 255 - .5) * cf, 0, 1), gm);
        if (ha > 0) {
            double s = clamp((l - .6) / .4, 0, 1);
            s = s * s * (3 - 2 * s);
            l = l * (1 - s) + std::pow(l, 1 + 1.5 * ha) * s;
        }
        double scale = L > .0001 ? l * 255 / L : 0;
        for (int c = 0; c < 3; ++c)
            a[i + c] = toU8Clamped(a[i + c] * scale);
    }
}
constexpr int b4[] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
constexpr int b8[] = {0,  32, 8,  40, 2,  34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4,  36, 14, 46,
                      6,  38, 60, 28, 52, 20, 62, 30, 54, 22, 3,  35, 11, 43, 1,  33, 9,  41, 51, 19, 59, 27,
                      49, 17, 57, 25, 15, 47, 7,  39, 13, 45, 5,  37, 63, 31, 55, 23, 61, 29, 53, 21};
void algorithm(std::vector<std::uint8_t> &a, int w, int h, const RenderParams &p, IProgress *progress) {
    bool pal = !p.palette.empty();
    bool inv = p.invert && !pal;
    if (p.algorithmId == 1) {
        // Two rows reproduce Float32Array writes exactly; all arithmetic before each write is double.
        int channels = pal ? 3 : 1;
        std::vector<float> rows(size_t(w) * channels * 2);
        auto load = [&](int y, int slot) {
            for (int x = 0; x < w; ++x) {
                size_t i = (size_t(y) * w + x) * 4, j = size_t(slot * w + x) * channels;
                if (pal) {
                    for (int c = 0; c < 3; ++c)
                        rows[j + c] = a[i + c];
                } else
                    rows[j] = float(.299 * a[i] + .587 * a[i + 1] + .114 * a[i + 2]);
            }
        };
        load(0, 0);
        for (int y = 0; y < h; ++y) {
            tick(progress, y, h);
            int s = y % 2, n = 1 - s;
            if (y + 1 < h)
                load(y + 1, n);
            int dir = p.serpentine && (y % 2) ? -1 : 1;
            for (int k = 0; k < w; ++k) {
                int x = dir == 1 ? k : w - 1 - k;
                size_t j = size_t(s * w + x) * channels, i = (size_t(y) * w + x) * 4;
                double e[3] = {};
                if (pal) {
                    double v[3];
                    for (int c = 0; c < 3; ++c)
                        v[c] = clamp(double(rows[j + c]) + ((p.threshold - 128) / 255.0) * 64);
                    size_t best = 0;
                    double bd = std::numeric_limits<double>::infinity();
                    for (size_t z = 0; z < p.palette.size(); ++z) {
                        auto q = p.palette[z];
                        double r = v[0] - q.r, g = v[1] - q.g, b = v[2] - q.b, d = r * r + g * g + b * b;
                        if (d < bd) {
                            bd = d;
                            best = z;
                        }
                    }
                    auto q = p.palette[best];
                    a[i] = q.r;
                    a[i + 1] = q.g;
                    a[i + 2] = q.b;
                    for (int c = 0; c < 3; ++c)
                        e[c] = v[c] - a[i + c];
                } else {
                    double v = rows[j], q = v < p.threshold ? 0 : 255;
                    e[0] = v - q;
                    a[i] = a[i + 1] = a[i + 2] = std::uint8_t(q);
                }
                a[i + 3] = 255;
                auto add = [&](int xx, int slot, double weight) {
                    if (xx < 0 || xx >= w)
                        return;
                    size_t z = size_t(slot * w + xx) * channels;
                    for (int c = 0; c < channels; ++c)
                        rows[z + c] = float(double(rows[z + c]) + e[c] * (weight * (p.errorStrength / 100)));
                };
                add(x + dir, s, 7.0 / 16);
                if (y + 1 < h) {
                    add(x - dir, n, 3.0 / 16);
                    add(x, n, 5.0 / 16);
                    add(x + dir, n, 1.0 / 16);
                }
            }
        }
    } else {
        for (int y = 0; y < h; ++y) {
            tick(progress, y, h);
            for (int x = 0; x < w; ++x) {
                size_t i = (size_t(y) * w + x) * 4;
                int g = a[i];
                Rgb8 q{};
                if (p.algorithmId == 15) {
                    int v = g < p.threshold ? 0 : 255;
                    if (pal && p.palette.size() >= 2)
                        q = p.palette[v ? 1 : 0];
                    else
                        q = {std::uint8_t(v), std::uint8_t(v), std::uint8_t(v)};
                } else {
                    double cell;
                    int n = p.orderedMatrix ? p.orderedMatrix : p.algorithmId == 2 ? 4 : 8;
                    if (p.algorithmId == 17)
                        cell = blueNoise[(y % 64) * 64 + x % 64] / 4095.0;
                    else
                        cell =
                            (n == 2 ? ((y%2)?((x%2)?1:3):((x%2)?2:0)) : n == 4 ? b4[(y % 4) * 4 + x % 4] : b8[(y % 8) * 8 + x % 8]) / double(n * n - 1);
                    if (pal) {
                        double base = g / 255.0 + ((p.threshold - 128) / 255.0) * 2;
                        if (p.algorithmId == 17)
                            base = clamp(base, 0, 1);
                        double adj = base + (cell - .5) / p.palette.size();
                        int pi = int(std::floor(adj * (p.palette.size() - 1) + .5));
                        q = p.palette[std::clamp(pi, 0, int(p.palette.size() - 1))];
                    } else {
                        double mt = p.algorithmId == 17
                                        ? cell * 255
                                        : (n == 4 ? b4[(y % 4) * 4 + x % 4] : b8[(y % 8) * 8 + x % 8]) *
                                              (255.0 / (n * n));
                        int v = g < clamp(mt + p.threshold - 128) ? 0 : 255;
                        q = {std::uint8_t(v), std::uint8_t(v), std::uint8_t(v)};
                    }
                }
                a[i] = q.r;
                a[i + 1] = q.g;
                a[i + 2] = q.b;
                a[i + 3] = 255;
            }
        }
    }
    if (inv)
        for (size_t i = 0; i < a.size(); i += 4)
            for (int c = 0; c < 3; ++c)
                a[i + c] = 255 - a[i + c];
}
} // namespace
RenderResult render(ConstImageView s, ImageView d, const RenderParams &p, IProgress *progress) noexcept {
    try {
        if (!s.data || !d.data || s.width <= 0 || s.height <= 0 || s.width != d.width || s.height != d.height)
            throw Failure{ErrorCode::InvalidImage, "invalid image dimensions/data"};
        auto valid = [](PixelFormat f) { return f == PixelFormat::RGBA8 || f == PixelFormat::RGBA16; };
        if (!valid(s.format) || !valid(d.format))
            throw Failure{ErrorCode::UnsupportedFormat, "unsupported pixel format"};
        size_t sw = size_t(s.width) * (s.format == PixelFormat::RGBA8 ? 4 : 8),
               dw = size_t(d.width) * (d.format == PixelFormat::RGBA8 ? 4 : 8);
        if (s.rowBytes < sw || d.rowBytes < dw || size_t(s.height) > SIZE_MAX / s.rowBytes ||
            size_t(d.height) > SIZE_MAX / d.rowBytes)
            throw Failure{ErrorCode::InvalidStride, "invalid rowBytes"};
        if(p.orderedMatrix!=0&&p.orderedMatrix!=2&&p.orderedMatrix!=4&&p.orderedMatrix!=8)return {false,"invalid ordered matrix",ErrorCode::InvalidParams};
        if (p.algorithmId != 1 && p.algorithmId != 2 && p.algorithmId != 8 && p.algorithmId != 15 &&
            p.algorithmId != 17 && p.algorithmId != 31)
            throw Failure{ErrorCode::UnsupportedAlgorithm, "algorithm not implemented"};
        if (p.threshold < 0 || p.threshold > 255 || !std::isfinite(p.errorStrength) || p.errorStrength < 0 ||
            p.errorStrength > 150)
            throw Failure{ErrorCode::InvalidParams, "invalid parameters"};
        double scale = p.workingResolution > 0
                           ? std::min(1.0, double(p.workingResolution) / std::max(s.width, s.height))
                           : 1;
        int w = std::max(1, int(std::floor(s.width * scale + .5))),
            h = std::max(1, int(std::floor(s.height * scale + .5)));
        if (size_t(w) > SIZE_MAX / size_t(h) / 4)
            throw Failure{ErrorCode::Overflow, "image overflow"};
        std::vector<std::uint8_t> a(size_t(w) * h * 4);
        for (int y = 0; y < h; ++y) {
            tick(progress, y, h);
            for (int x = 0; x < w; ++x) {
                double sx = clamp((x + .5) * s.width / w - .5, 0, s.width - 1),
                       sy = clamp((y + .5) * s.height / h - .5, 0, s.height - 1);
                int x0 = int(sx), y0 = int(sy), x1 = std::min(x0 + 1, s.width - 1),
                    y1 = std::min(y0 + 1, s.height - 1);
                double fx = sx - x0, fy = sy - y0;
                for (int c = 0; c < 4; ++c) {
                    double top = read(s, x0, y0, c) * (1 - fx) + read(s, x1, y0, c) * fx,
                           bot = read(s, x0, y1, c) * (1 - fx) + read(s, x1, y1, c) * fx;
                    a[(size_t(y) * w + x) * 4 + c] = toU8Clamped(top * (1 - fy) + bot * fy);
                }
            }
        }
        if(p.algorithmId!=31)tone(a, p.tone);
        if (p.algorithmId!=31 && p.invert && !p.palette.empty())
            for (size_t i = 0; i < a.size(); i += 4)
                for (int c = 0; c < 3; ++c)
                    a[i + c] = 255 - a[i + c];
        if(p.algorithmId==31){auto result=p.smoothHalftone?applyPrintHalftone(a,w,h,p.halftonePitch,p.halftoneAngle,p.halftoneGain,progress):applyHalftone(a,w,h,p.halftonePitch,progress);if(!result.ok)return result;}
        else algorithm(a, w, h, p, progress);
        for (int y = 0; y < d.height; ++y) {
            tick(progress, y, d.height);
            auto row = static_cast<std::uint8_t *>(d.data) + y * d.rowBytes;
            for (int x = 0; x < d.width; ++x) {
                size_t i = (size_t(y) * h / d.height * w + size_t(x) * w / d.width) * 4;
                for (int c = 0; c < 4; ++c) {
                    auto v = c == 3 && (p.alphaMode == AlphaMode::Preserve||p.algorithmId==31) ? read(s, x, y, 3) : a[i + c];
                    if (d.format == PixelFormat::RGBA8)
                        row[x * 4 + c] = v;
                    else {
                        std::uint16_t v16 = v * 257;
                        std::memcpy(row + (x * 4 + c) * 2, &v16, 2);
                    }
                }
            }
        }
        if (progress)
            progress->report(1);
        return {true, {}};
    } catch (const Failure &e) {
        return failure(e.code, e.message);
    } catch (const std::bad_alloc &) {
        return failure(ErrorCode::OutOfMemory, "out of memory");
    } catch (...) {
        return failure(ErrorCode::Internal, "unknown render error");
    }
}
} // namespace dither
