// PA-M1 core checks: logical grid geometry, alpha-weighted sampling, OKLab nearest colour,
// nearest-neighbour reconstruction and the generated F01/F02/F08 fixtures.
#include "pixel_art/core.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pa = pixel_art;
namespace {
int failures = 0;
void check(bool ok, const std::string &name) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL " << name << '\n';
    }
}
bool near(double a, double b, double tolerance = 1e-6) { return std::fabs(a - b) <= tolerance; }

struct Image8 {
    int w = 0, h = 0;
    std::vector<unsigned char> data;
    Image8(int width, int height) : w(width), h(height), data(std::size_t(width) * height * 4, 0) {}
    pa::ConstImageView in(int ox = 0, int oy = 0) const {
        return {data.data(), w, h, std::size_t(w) * 4, pa::PixelFormat::RGBA8, double(ox), double(oy)};
    }
    pa::ImageView out(int ox = 0, int oy = 0) {
        return {data.data(), w, h, std::size_t(w) * 4, pa::PixelFormat::RGBA8, double(ox), double(oy)};
    }
    void set(int x, int y, int r, int g, int b, int a = 255) {
        auto *p = data.data() + (std::size_t(y) * w + x) * 4;
        p[0] = (unsigned char)r; p[1] = (unsigned char)g; p[2] = (unsigned char)b; p[3] = (unsigned char)a;
    }
    const unsigned char *at(int x, int y) const { return data.data() + (std::size_t(y) * w + x) * 4; }
};

const pa::Rgb8 black{0, 0, 0}, white{255, 255, 255}, red{255, 0, 0}, green{0, 255, 0},
    blue{0, 0, 255};

pa::Params params(std::vector<pa::Rgb8> palette, int block) {
    pa::Params p;
    p.palette = std::move(palette);
    p.grid.block = block;
    return p;
}
bool same(const unsigned char *pixel, pa::Rgb8 color, int tolerance = 0) {
    for (int c = 0; c < 3; ++c) {
        int expected = c == 0 ? color.r : c == 1 ? color.g : color.b;
        if (std::abs(int(pixel[c]) - expected) > tolerance)
            return false;
    }
    return true;
}
std::size_t uniqueColors(const Image8 &image) {
    std::vector<int> seen;
    for (int y = 0; y < image.h; ++y)
        for (int x = 0; x < image.w; ++x) {
            const auto *p = image.at(x, y);
            if (p[3] == 0)
                continue;
            int key = (p[0] << 16) | (p[1] << 8) | p[2];
            bool known = false;
            for (int v : seen)
                if (v == key)
                    known = true;
            if (!known)
                seen.push_back(key);
        }
    return seen.size();
}
void fillGradient(Image8 &image) {
    // F01: horizontal hue ramp plus a vertical luminance ramp.
    for (int y = 0; y < image.h; ++y)
        for (int x = 0; x < image.w; ++x)
            image.set(x, y, x * 255 / (image.w - 1), y * 255 / (image.h - 1),
                      255 - x * 255 / (image.w - 1));
}
void fillLines(Image8 &image) {
    // F02: 1 px vertical and horizontal lines, a diagonal and a highlight block.
    for (int y = 0; y < image.h; ++y)
        for (int x = 0; x < image.w; ++x)
            image.set(x, y, 32, 32, 32);
    for (int y = 0; y < image.h; ++y)
        image.set(image.w / 2, y, 240, 240, 240);
    for (int x = 0; x < image.w; ++x)
        image.set(x, image.h / 3, 240, 240, 240);
    for (int i = 0; i < std::min(image.w, image.h); ++i)
        image.set(i, i, 200, 60, 60);
    image.set(2, 2, 255, 255, 0);
    image.set(3, 2, 255, 255, 0);
}
void fillQuadrants(Image8 &image) {
    for (int y = 0; y < image.h; ++y)
        for (int x = 0; x < image.w; ++x) {
            if (y < image.h / 2)
                image.set(x, y, x < image.w / 2 ? 255 : 0, x < image.w / 2 ? 0 : 255, 0);
            else
                image.set(x, y, x < image.w / 2 ? 0 : 255, x < image.w / 2 ? 0 : 255,
                          x < image.w / 2 ? 255 : 255);
        }
}

struct CancelAfter : pa::IProgress {
    int reports = 0;
    bool cancelled() override { return reports >= 1; }
    void report(double) override { ++reports; }
};
} // namespace

int main() {
    // --- colour science -------------------------------------------------------------
    check(near(pa::oklabFromSrgb(white).l, 1.0, 2e-3), "oklab white lightness");
    check(near(pa::oklabFromSrgb(black).l, 0.0, 2e-3), "oklab black lightness");
    check(near(pa::oklabFromSrgb(pa::Rgb8{128, 128, 128}).l, 0.598, 3e-3), "oklab mid grey lightness");
    check(near(pa::oklabFromSrgb(white).a, 0.0, 3e-3), "oklab white a is neutral");
    check(near(pa::oklabFromSrgb(white).b, 0.0, 3e-3), "oklab white b is neutral");
    for (int v = 0; v <= 255; v += 15) {
        double round = pa::linearToSrgb(pa::srgbToLinear(v / 255.0)) * 255.0;
        check(std::fabs(round - v) < 0.01, "srgb round trip " + std::to_string(v));
    }

    // --- F08: degenerate sizes, pure colour, transparency ---------------------------
    {
        Image8 one(1, 1);
        one.set(0, 0, 200, 30, 30);
        auto result = pa::render(one.in(), one.out(), params({red, blue}, 4));
        check(result.ok, "1x1 render ok");
        check(same(one.at(0, 0), red), "1x1 maps to nearest palette colour");
        check(one.at(0, 0)[3] == 255, "1x1 keeps opacity");
    }
    {
        Image8 strip(64, 1);
        for (int x = 0; x < 64; ++x)
            strip.set(x, 0, x * 4, 0, 0);
        auto result = pa::render(strip.in(), strip.out(), params({black, white}, 1));
        check(result.ok, "1 px tall strip ok");
        check(uniqueColors(strip) <= 2, "strip uses at most the palette size");
    }
    {
        Image8 clear(8, 8);
        auto result = pa::render(clear.in(), clear.out(), params({black, white}, 2));
        check(result.ok, "transparent image ok");
        bool allClear = true;
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                allClear = allClear && clear.at(x, y)[3] == 0 && clear.at(x, y)[0] == 0;
        check(allClear, "transparent stays clear without hidden colour");
    }

    // --- F01: gradient bars stay inside the palette and are deterministic ----------
    {
        Image8 source(32, 16);
        fillGradient(source);
        std::vector<pa::Rgb8> palette = {black, pa::Rgb8{128, 128, 128}, white, red, green, blue};
        auto p = params(palette, 2);
        Image8 first(32, 16), second(32, 16);
        auto result = pa::render(source.in(), first.out(), p);
        auto again = pa::render(source.in(), second.out(), p);
        check(result.ok && again.ok, "gradient render ok");
        check(uniqueColors(first) <= palette.size(), "gradient colour count bounded by palette");
        check(first.data == second.data, "same input and params are reproducible");
    }

    // --- F02: 1 px structure survives at block 1 ----------------------------------
    {
        Image8 image(16, 16);
        fillLines(image);
        auto result = pa::render(image.in(), image.out(), params({black, white, red}, 1));
        check(result.ok, "line fixture ok");
        check(same(image.at(8, 4), white), "1 px vertical line preserved");
        check(same(image.at(4, 5), white), "1 px horizontal line preserved");
        check(!same(image.at(2, 2), black), "highlight is not dropped");
    }

    // --- logical grid geometry ------------------------------------------------------
    {
        Image8 image(8, 8);
        fillQuadrants(image);
        auto result = pa::render(image.in(), image.out(), params({red, green, blue, white}, 4));
        check(result.ok, "block render ok");
        bool uniform = true;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                uniform = uniform && std::memcmp(image.at(0, 0), image.at(x, y), 3) == 0;
        check(uniform, "4x4 block is uniform");
        check(same(image.at(0, 0), red), "top-left block keeps its quadrant colour");
        check(same(image.at(4, 0), green), "top-right block keeps its quadrant colour");
        check(same(image.at(0, 4), blue), "bottom-left block keeps its quadrant colour");
        check(same(image.at(4, 4), white), "bottom-right block keeps its quadrant colour");
    }
    {
        // A non-zero origin must move the grid phase, not silently re-anchor at 0.
        Image8 a(8, 8), b(8, 8);
        fillQuadrants(a);
        fillQuadrants(b);
        auto p = params({red, green, blue, white}, 4);
        pa::render(a.in(0, 0), a.out(0, 0), p);
        pa::render(b.in(2, 0), b.out(2, 0), p);
        check(std::memcmp(a.at(2, 0), b.at(2, 0), 3) != 0, "origin changes the sampling phase");
    }
    {
        // A four-column cell at PAR=2 is eight display pixels wide and eight rows high.
        Image8 image(8, 8);
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x) {
                if (y < 2) image.set(x, y, 255, 0, 0);
                else if (y < 4) image.set(x, y, 0, 255, 0);
                else if (y < 6) image.set(x, y, 0, 0, 255);
                else image.set(x, y, 255, 255, 255);
            }
        auto p = params({red, green, blue, white}, 4);
        p.grid.pixelAspect = 2.0;
        auto result = pa::render(image.in(), image.out(), p);
        check(result.ok, "pixel aspect render ok");
        check(std::memcmp(image.at(0,0),image.at(0,7),4)==0,
              "PAR 2 doubles vertical coverage to keep display-space cells square");
    }

    // --- alpha-weighted sampling ----------------------------------------------------
    {
        Image8 image(4, 4);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                if (x < 2)
                    image.set(x, y, 255, 0, 0, 255);
                else
                    image.set(x, y, 0, 255, 0, 0); // hidden green must not leak
        auto result = pa::render(image.in(), image.out(), params({red, green, black}, 4));
        check(result.ok, "alpha weighting ok");
        check(same(image.at(0, 0), red), "transparent hidden colour ignored");
        check(std::abs(int(image.at(0, 0)[3]) - 128) <= 1, "coverage alpha is 50%");
    }
    {
        Image8 image(4, 4);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                image.set(x, y, 255, 0, 0, x < 2 ? 255 : 0);
        auto p = params({red, green, black}, 4);
        p.binaryAlpha = true;
        p.alphaThreshold = 0.75;
        auto result = pa::render(image.in(), image.out(), p);
        check(result.ok, "binary alpha ok");
        check(image.at(0, 0)[3] == 0 && image.at(0, 0)[0] == 0, "binary alpha clears hidden colour");
    }

    // --- 16 bpc input and output ----------------------------------------------------
    {
        const int w = 8, h = 8;
        std::vector<unsigned char> src(std::size_t(w) * h * 8), dst(std::size_t(w) * h * 8, 0);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                auto *p = src.data() + (std::size_t(y) * w + x) * 8;
                std::uint16_t values[4] = {std::uint16_t(x * 8191), std::uint16_t(y * 8191), 0, 65535};
                std::memcpy(p, values, 8);
            }
        pa::ConstImageView in{src.data(), w, h, std::size_t(w) * 8, pa::PixelFormat::RGBA16, 0, 0};
        pa::ImageView out{dst.data(), w, h, std::size_t(w) * 8, pa::PixelFormat::RGBA16, 0, 0};
        auto result = pa::render(in, out, params({black, white, red, green, blue}, 2));
        check(result.ok, "16 bpc render ok");
        std::uint16_t alpha = 0;
        std::memcpy(&alpha, dst.data() + 6, 2);
        check(alpha == 65535, "16 bpc keeps full alpha");
    }

    // --- rejection paths ------------------------------------------------------------
    {
        Image8 image(4, 4);
        check(pa::render(image.in(), image.out(), params({}, 1)).code == pa::ErrorCode::EmptyPalette,
              "empty palette rejected");
        check(pa::render(image.in(), image.out(), params({black}, 0)).code == pa::ErrorCode::InvalidParams,
              "block 0 rejected");
        check(pa::render(image.in(), image.out(), params({black}, 65)).code == pa::ErrorCode::InvalidParams,
              "block 65 rejected");
        auto p = params({black}, 2);
        p.grid.pixelAspect = std::nan("");
        check(pa::render(image.in(), image.out(), p).code == pa::ErrorCode::InvalidParams,
              "NaN pixel aspect rejected");
        p = params({black}, 2);
        p.alphaThreshold = 2;
        check(pa::render(image.in(), image.out(), p).code == pa::ErrorCode::InvalidParams,
              "alpha threshold out of range rejected");
        auto bad = image.in();
        bad.rowBytes = 4; // narrower than one RGBA8 row
        check(pa::render(bad, image.out(), params({black}, 2)).code == pa::ErrorCode::InvalidStride,
              "short rowBytes rejected");
        auto nullImage = image.in();
        nullImage.data = nullptr;
        check(pa::render(nullImage, image.out(), params({black}, 2)).code == pa::ErrorCode::InvalidImage,
              "null source rejected");
    }

    // --- cancellation ---------------------------------------------------------------
    {
        Image8 image(64, 64);
        fillGradient(image);
        CancelAfter progress;
        auto result = pa::render(image.in(), image.out(), params({black, white}, 2), &progress);
        check(!result.ok && result.code == pa::ErrorCode::Cancelled, "cancellation is reported");
    }

    if (failures) {
        std::cerr << "pixel art core: " << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "pixel art core: geometry, alpha-weighted sampling, OKLab mapping, "
                 "16 bpc, rejection and cancellation passed.\n";
    return 0;
}
