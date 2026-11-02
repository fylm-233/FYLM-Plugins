// Pure color helpers shared by pixel-art strategies.
#include "pixel_art/core.hpp"
#include <algorithm>
#include <cmath>
namespace pixel_art {
std::uint8_t toU8(double value) noexcept {
    if (!(value > 0))
        return 0;
    if (value >= 255)
        return 255;
    return static_cast<std::uint8_t>(std::lround(value));
}

double srgbToLinear(double value) noexcept {
    if (!(value > 0))
        return 0;
    if (value >= 1)
        return 1;
    return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
}

double linearToSrgb(double value) noexcept {
    if (!(value > 0))
        return 0;
    if (value >= 1)
        return 1;
    return value <= 0.0031308 ? value * 12.92 : 1.055 * std::pow(value, 1 / 2.4) - 0.055;
}

Lab oklabFromLinear(double r, double g, double b) noexcept {
    // Björn Ottosson's OKLab transform, applied to linear sRGB.
    const double l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b;
    const double m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b;
    const double s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b;
    const double l_ = std::cbrt(l), m_ = std::cbrt(m), s_ = std::cbrt(s);
    Lab result;
    result.l = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_;
    result.a = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_;
    result.b = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_;
    return result;
}

Lab oklabFromSrgb(Rgb8 color) noexcept {
    return oklabFromLinear(srgbToLinear(color.r / 255.0), srgbToLinear(color.g / 255.0),
                           srgbToLinear(color.b / 255.0));
}

double labDistanceSquared(const Lab &a, const Lab &b) noexcept {
    const double dl = a.l - b.l, da = a.a - b.a, db = a.b - b.b;
    return dl * dl + da * da + db * db;
}

const char *errorKey(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::None:
        return "";
    case ErrorCode::InvalidImage:
        return "pa.error.invalid_image";
    case ErrorCode::UnsupportedFormat:
        return "pa.error.unsupported_format";
    case ErrorCode::InvalidStride:
        return "pa.error.invalid_stride";
    case ErrorCode::InvalidParams:
        return "pa.error.invalid_params";
    case ErrorCode::EmptyPalette:
        return "pa.error.empty_palette";
    case ErrorCode::Overflow:
        return "pa.error.overflow";
    case ErrorCode::OutOfMemory:
        return "pa.error.memory";
    case ErrorCode::Cancelled:
        return "pa.error.cancelled";
    default:
        return "pa.error.internal";
    }
}


} // namespace pixel_art
