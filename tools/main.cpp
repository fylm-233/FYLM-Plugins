#include "dither/core.hpp"
#include "dither/i18n.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
int main(int argc, char **argv) {
    namespace loc = dither::i18n;
    auto locale = loc::Locale::English;
    if (argc >= 3 && std::string(argv[1]) == "--lang") {
        locale = loc::fromCode(argv[2]);
        argc -= 2;
        argv += 2;
    }
    const char *key = "error.arguments";
    try {
        if (argc < 5) {
            std::cerr << loc::text(locale, "cli.usage") << '\n';
            return 2;
        }
        int w = std::stoi(argv[3]), h = std::stoi(argv[4]);
        key = "error.invalid_image";
        if (w <= 0 || h <= 0 || size_t(w) > SIZE_MAX / size_t(h) / 4)
            throw std::runtime_error("invalid size");
        size_t n = size_t(w) * h * 4;
        std::vector<unsigned char> src(n), dst(n);
        std::ifstream in(argv[1], std::ios::binary);
        key = "error.input_size";
        if (!in.read((char *)src.data(), n) || in.peek() != EOF)
            throw std::runtime_error("input must contain exactly WIDTH*HEIGHT*4 bytes");
        dither::RenderParams p;
        key = "error.arguments";
        if (argc > 5)
            p.algorithmId = std::stoi(argv[5]);
        if (argc > 6) {
            key = "error.palette";
            p.palette = dither::palette(argv[6]);
        }
        key = "error.arguments";
        if (argc > 7)
            p.threshold = std::stoi(argv[7]);
        if (argc > 8)
            p.workingResolution = std::stoi(argv[8]);
        if (argc > 9)
            p.invert = std::stoi(argv[9]) != 0;
        if (argc > 10)
            p.serpentine = std::stoi(argv[10]) != 0;
        auto r = dither::render({src.data(), w, h, size_t(w) * 4, dither::PixelFormat::RGBA8},
                                {dst.data(), w, h, size_t(w) * 4, dither::PixelFormat::RGBA8}, p);
        if (!r.ok) {
            key = dither::errorKey(r.code);
            throw std::runtime_error(r.error);
        }
        std::ofstream out(argv[2], std::ios::binary);
        key = "error.output";
        if (!out.write((char *)dst.data(), n))
            throw std::runtime_error("cannot write output");
        return 0;
    } catch (const std::bad_alloc &) {
        std::cerr << loc::text(locale, "error.memory") << '\n';
        return 1;
    } catch (const std::exception &) {
        std::cerr << loc::text(locale, key) << '\n';
        return 1;
    }
}
