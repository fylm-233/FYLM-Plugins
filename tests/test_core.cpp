#include "dither/core.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace dither;
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
std::vector<unsigned char> file(std::string p) {
    std::ifstream f(p, std::ios::binary);
    check(bool(f), "fixture missing");
    return {std::istreambuf_iterator<char>(f), {}};
}
int main() {
    try {
        check(toU8Clamped(-1) == 0 && toU8Clamped(300) == 255 && toU8Clamped(NAN) == 0, "clamp bounds");
        check(toU8Clamped(.5) == 0 && toU8Clamped(1.5) == 2 && toU8Clamped(2.5) == 2, "ties even");
        int count = 0;
        std::ifstream cases(std::string(GOLDEN_DIR) + "/cases.txt");
        check(bool(cases), "cases missing");
        std::string name, pal;
        int w, h, id, t, serp, inv;
        ToneParams tone;
        while (cases >> name >> w >> h >> id >> t >> serp >> inv >> pal >> tone.contrast >> tone.midtones >>
               tone.highlights) {
            auto src = file(std::string(GOLDEN_DIR) + "/" + name + ".in.rgba"),
                 expected = file(std::string(GOLDEN_DIR) + "/" + name + ".out.rgba");
            std::vector<unsigned char> out(src.size());
            RenderParams p;
            p.algorithmId = id;
            p.threshold = t;
            p.serpentine = serp != 0;
            p.invert = inv != 0;
            p.palette = palette(pal);
            p.tone = tone;
            p.workingResolution = 0;
            auto r = render({src.data(), w, h, size_t(w) * 4, PixelFormat::RGBA8},
                            {out.data(), w, h, size_t(w) * 4, PixelFormat::RGBA8}, p);
            check(r.ok, r.error.c_str());
            if (out != expected) {
                std::cerr << "golden mismatch: " << name << " algorithm " << id << '\n';
                return 1;
            }
            ++count;
        }
        check(count == 480, "fixture count");
        for (int id : {1, 2, 8, 15, 17})
            for (int w : {1, 2, 17})
                for (int h : {1, 2, 13}) {
                    size_t stride = w * 4 + 7;
                    std::vector<unsigned char> s(stride * h, 0xAD), d(stride * h, 0xCD);
                    for (int y = 0; y < h; y++)
                        for (int x = 0; x < w; x++) {
                            auto i = y * stride + x * 4;
                            s[i] = s[i + 1] = s[i + 2] = (x * 31 + y * 17) % 256;
                            s[i + 3] = (x + y) % 3 * 127;
                        }
                    RenderParams p;
                    p.algorithmId = id;
                    p.alphaMode = AlphaMode::Preserve;
                    p.workingResolution = 3;
                    check(render({s.data(), w, h, stride, PixelFormat::RGBA8},
                                 {d.data(), w, h, stride, PixelFormat::RGBA8}, p)
                              .ok,
                          "stride render");
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w; x++)
                            check(d[y * stride + x * 4 + 3] == s[y * stride + x * 4 + 3], "alpha");
                        for (size_t x = w * 4; x < stride; x++)
                            check(d[y * stride + x] == 0xCD, "padding overwritten");
                    }
                }
        std::uint16_t s16[] = {65535, 65535, 65535, 32768}, d16[4] = {};
        RenderParams p;
        p.algorithmId = 15;
        p.alphaMode = AlphaMode::Preserve;
        check(render({s16, 1, 1, 8, PixelFormat::RGBA16}, {d16, 1, 1, 8, PixelFormat::RGBA16}, p).ok,
              "16bit");
        check(d16[0] == 65535 && d16[3] == 32896, "16bit compatibility");
        check(!render({s16, 1, 1, 1, PixelFormat::RGBA16}, {d16, 1, 1, 8, PixelFormat::RGBA16}, p).ok,
              "invalid stride rejected");
        std::cout << count
                  << " golden cases byte-identical; rounding, odd dimensions, padding, alpha, RGBA16 and "
                     "validation passed.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
