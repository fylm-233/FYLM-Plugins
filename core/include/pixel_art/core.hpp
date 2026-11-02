#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// FYLM Pixel Art core contract (PA-M1). Pure algorithm: no AE, file, clock or UI access.
// All geometry is expressed in the host layer's full coordinate space, so ROI, non-zero
// origin and pixel aspect ratio stay consistent between host and CLI callers.
namespace pixel_art {

// ARGB variants are output-only, premultiplied, with the stated integer scale.
enum class PixelFormat { RGBA8, RGBA16, RGBA32F, ARGB8Premultiplied, ARGB16_32768Premultiplied, ARGB32FPremultiplied };

// RGBA formats are straight; output-only ARGB formats use premultiplied alpha.
struct ConstImageView {
    const void *data = nullptr;
    int width = 0, height = 0;
    std::size_t rowBytes = 0;
    PixelFormat format = PixelFormat::RGBA8;
    // Full-coordinate position of the top-left pixel; not part of the buffer.
    double originX = 0, originY = 0;
};
struct ImageView {
    void *data = nullptr;
    int width = 0, height = 0;
    std::size_t rowBytes = 0;
    PixelFormat format = PixelFormat::RGBA8;
    double originX = 0, originY = 0;
};

struct Rgb8 {
    std::uint8_t r = 0, g = 0, b = 0;
};
struct Lab {
    double l = 0, a = 0, b = 0;
};

enum class Sampling {
    Area,   // alpha-weighted box average over the logical cell
    Nearest, // single sample at the cell centre
    Feature // bounded representative-color sampling, with connected thin-feature support
};
// Only sRGB and linear sRGB are defined. No arbitrary working-space claim is made.
enum class ColorEncoding { Srgb, LinearSrgb };

struct Grid {
    int block = 4;                 // full-resolution layer columns per block, 1..64
    int originX = 0, originY = 0;  // full-coordinate anchor of logical cell (0,0)
    double pixelAspect = 1.0;      // block height = block * PAR: square in display space
    double downsampleX = 1, downsampleY = 1; // buffer samples per full-resolution pixel
    Sampling sampling = Sampling::Area;
};

enum class Style { Basic, GBA, Modern };
// Hardware colour model, separate from AE's per-channel input/output precision.
// Art preserves the historical 2..16-colour FYLM presets exactly.
enum class PC98ColorSpec { Art=0, Digital8, Analog16, Indexed256, RGB565, RGB888 };
struct Params {
    Grid grid;
    ColorEncoding encoding = ColorEncoding::Srgb;
    std::vector<Rgb8> palette; // Art requires entries; hardware indexed can use a reference default; direct colour ignores it.
    bool binaryAlpha = false;
    double alphaThreshold = 0.5;
    bool pc98 = false;
    PC98ColorSpec pc98ColorSpec = PC98ColorSpec::Art;
    int pc98PaletteBits = 8; // Indexed256 DAC: 6 or 8 bits/channel (18 or 24 RGB bits).
    int ditherMatrix = 4; // 0, 2, 4, 8
    double ditherStrength = 0.65, edgeProtection = 0.8;
    Style style = Style::Basic; // PC-98 keeps its original pc98 flag and exact path.
    double featureStrength = .65, outlineStrength = 0;
    double mergeThreshold = .042, detailProtection = .8;
    int maxClusterArea = 4, cleanupIterations = 2;
    bool cleanSpeckles = true, cleanSpurs = true, cleanStairs = false;
    bool debugChanges = false;
    int workers = 1; // 0=hardware auto, 1=serial, 2..8=requested ceiling, never a guarantee.
    std::size_t memoryBudget = 256ull*1024*1024;
};

enum class ErrorCode {
    None,
    InvalidImage,
    UnsupportedFormat,
    InvalidStride,
    InvalidParams,
    EmptyPalette,
    Overflow,
    OutOfMemory,
    Cancelled,
    Internal
};
const char *errorKey(ErrorCode code) noexcept;

struct Result {
    bool ok = false;
    ErrorCode code = ErrorCode::None;
    std::string error;
};

class IProgress {
  public:
    virtual ~IProgress() = default;
    virtual bool cancelled() = 0;
    virtual void report(double fraction) = 0;
};

std::uint8_t toU8(double value) noexcept;
double srgbToLinear(double value) noexcept;
double linearToSrgb(double value) noexcept;
Lab oklabFromLinear(double r, double g, double b) noexcept;
Lab oklabFromSrgb(Rgb8 color) noexcept;
std::vector<Rgb8> pc98Palette();
std::vector<Rgb8> pc98HardwarePalette(PC98ColorSpec spec,int count=256,int dacBits=8);
std::vector<Rgb8> pc98ConformPalette(const std::vector<Rgb8>& colors,PC98ColorSpec spec,int dacBits=8);
// Encoded sRGB input, ordered offset in [-.5,.5]; no palette allocation.
Rgb8 pc98DirectColor(double r,double g,double b,PC98ColorSpec spec,double orderedOffset=0) noexcept;
std::vector<Rgb8> stylePalette(Style style);
// Deterministic weighted median cut. Called on an explicitly checked-out reference
// frame by the host, never on implicit previous-frame state.
std::vector<Rgb8> extractPalette(ConstImageView src, ColorEncoding encoding, int colors,
                                IProgress* progress=nullptr);
double labDistanceSquared(const Lab &a, const Lab &b) noexcept;

// Logical-grid resample, palette mapping and optional PC-98 ordered dithering.
// GBA and modern styles share sampling and palette logic; structure edits operate
// on immutable label snapshots, never on implicit previous-frame data.
Result render(ConstImageView src, ImageView dst, const Params &params,
              IProgress *progress = nullptr) noexcept;
} // namespace pixel_art
