#include "retro/retro.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <set>
#include <regex>
void check(bool b) {
    if (!b)
        throw std::runtime_error("retro integration invariant failed");
}
struct Sink {
    int next = 0;
    std::map<int, std::string> names;
    std::map<std::string, double> values;
};
int locate(void *c, std::uint32_t, const char *s) {
    auto &t = *static_cast<Sink *>(c);
    int l = t.next++;
    t.names[l] = s;
    return l;
}
void oneF(void *c, int l, float v) {
    auto &t = *static_cast<Sink *>(c);
    t.values[t.names[l]] = v;
}
void oneI(void *c, int l, int v) { oneF(c, l, float(v)); }
void twoF(void *c, int l, float x, float y) {
    auto &t = *static_cast<Sink *>(c);
    t.values[t.names[l] + ".x"] = x;
    t.values[t.names[l] + ".y"] = y;
}
void twoI(void *c, int l, int x, int y) { twoF(c, l, float(x), float(y)); }
int main() {
    try {
        retro::Controls c;
        std::string error;
        check(retro::validate(c, error));
        c.retroEnabled = true;
        auto p = retro::makePlan(c, {1920, 1080});
        check(p.ok && p.stages.size() == 5);
        check(p.stages[0].pass == retro::Pass::DownsampleLinear && p.stages[0].size.width == 512 &&
              p.stages[0].size.height == 288);
        check(p.stages[3].pass == retro::Pass::VHS && p.stages[4].pass == retro::Pass::CRT);
        c.order = retro::Order::RetroThenDither;
        p = retro::makePlan(c, {1920, 1080});
        check(p.ok && p.stages[0].pass == retro::Pass::VHS &&
              p.stages.back().pass == retro::Pass::UpsampleNearest);
        c.dither.enabled = 0;
        p = retro::makePlan(c, {1920, 1080});
        check(p.ok && p.stages.size() == 2 && p.stages[0].pass == retro::Pass::VHS &&
              p.stages[1].pass == retro::Pass::CRT);
        c.retroEnabled = false;
        p = retro::makePlan(c, {1920, 1080});
        check(p.ok && p.stages.empty());
        c.retroEnabled = true;
        c.crt.vhsEnabled = 0;
        c.crt.crtEnabled = 0;
        p = retro::makePlan(c, {1920, 1080});
        check(p.ok && p.stages.empty());
        c.dither.enabled = 1;
        c.crt.vhsEnabled = 1;
        c.crt.crtEnabled = 1;
        auto f = retro::makeUniformFrame(c, {1920, 1080}, 1001, 24000);
        check(std::abs(f.timeSeconds - 1001.0 / 24000) < 1e-7);
        c.animateTime = false;
        c.timeOffset = 3;
        check(retro::makeUniformFrame(c, {1920, 1080}, -1001, 24000).timeSeconds == 3);
        c.dither.algorithmId = 1;
        check(!retro::makePlan(c, {64, 64}).ok);
        c.dither.enabled = 0;
        check(retro::makePlan(c, {64, 64}).ok);
        c.dither.enabled = 1;
        c.dither.algorithmId = 2;
        c.crt.curvature = std::nanf("");
        check(!retro::makePlan(c, {64, 64}).ok);
        c.crt.curvature = 0;
        Sink sink;
        retro::UniformApi api{&sink, locate, oneF, oneI, twoF, twoI};
        auto locations = retro::locateUniforms(api, 1);
        f = retro::makeUniformFrame(c, {1920, 1080}, 0, 1);
        retro::bindUniforms(api, locations, f, 13, 17);
        check(sink.values["uInput"] == 0 && sink.values["uPalette"] == 1 && sink.values["uNoiseRanks"] == 2);
        check(sink.values["uResolution.x"] == 1920 && sink.values["uTime"] == 3 &&
              sink.values["uWorkOrigin.x"] == 13);
        check(sink.values.count("uCrt.borderSmoothness") && sink.values.count("uCrt.noiseColor"));
        std::set<std::uint16_t> ranks(retro::blueNoiseRanking(), retro::blueNoiseRanking() + 4096);
        check(ranks.size() == 4096 && *ranks.begin() == 0 && *ranks.rbegin() == 4095);
        // Cross-language contract: every declared GLSL parameter has a named host binding.
        std::set<std::string> bound;
        for (const auto &e : locations.entries)
            bound.insert(e.name);
        auto checkSchema = [&](const char *source, const char *tag, const char *prefix) {
            std::string s(source);
            auto start = s.find(std::string("struct ") + tag);
            check(start != std::string::npos);
            auto end = s.find("};", start);
            check(end != std::string::npos);
            s = s.substr(start, end - start);
            std::regex field(R"((float|int)\s+([A-Za-z0-9_]+)\s*;)");
            int count = 0;
            for (std::sregex_iterator it(s.begin(), s.end(), field), last; it != last; ++it) {
                check(bound.count(std::string(prefix) + (*it)[2].str()) == 1);
                count++;
            }
            check(count > 0);
        };
        checkSchema(retro::crtShader(), "CRTVHSParams", "uCrt.");
        checkSchema(retro::ditherShader(), "DitherParams", "uDither.");
        for (int i = 0; i <= 6; i++) {
            auto text = retro::fragmentShader(i);
            check(text.find("#version 330 core") == 0 && text.find("SCREEN_TEXTURE") == std::string::npos &&
                  text.find("shader_type") == std::string::npos);
        }
        std::cout << "Independent dither/analog plans, pass order, rational time, FS rejection, uniform mapping and blue-noise "
                     "ranks passed.\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
