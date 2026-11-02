#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace dither {
enum class PixelFormat { RGBA8, RGBA16 };
struct ConstImageView {
    const void *data;
    int width, height;
    std::size_t rowBytes;
    PixelFormat format;
};
struct ImageView {
    void *data;
    int width, height;
    std::size_t rowBytes;
    PixelFormat format;
};
struct Rgb8 {
    std::uint8_t r, g, b;
};
enum class AlphaMode { Opaque, Preserve };
struct ToneParams {
    double contrast = 0, midtones = 1, highlights = 0;
};
struct RenderParams {
    int algorithmId = 1, threshold = 128;
    bool invert = false, serpentine = true;
    int workingResolution = 512;
    float halftonePitch = 2.5f;
    ToneParams tone;
    double errorStrength = 100;
    std::vector<Rgb8> palette;
    AlphaMode alphaMode = AlphaMode::Opaque;
};
enum class ErrorCode {
    None,
    InvalidImage,
    UnsupportedFormat,
    InvalidStride,
    UnsupportedAlgorithm,
    InvalidParams,
    Overflow,
    Cancelled,
    OutOfMemory,
    Internal
};
const char *errorKey(ErrorCode code) noexcept;
struct RenderResult {
    bool ok;
    std::string error;
    ErrorCode code = ErrorCode::None;
};
class IProgress {
  public:
    virtual ~IProgress() = default;
    virtual bool cancelled() = 0;
    virtual void report(double) = 0;
};
std::uint8_t toU8Clamped(double v) noexcept;
std::vector<Rgb8> palette(const std::string &id);
RenderResult render(ConstImageView src, ImageView dst, const RenderParams &params,
                    IProgress *progress = nullptr) noexcept;
} // namespace dither
