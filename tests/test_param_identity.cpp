// Frozen AE parameter identity for FYLM Pixel Art (formerly FYLM Dithering Studio).
// Old projects, expressions and scripts address parameters by index and type, so any
// insertion, reordering or default change silently breaks them. This test fails loudly.
#include "../plugin/ae/FYLMParams.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
struct Expected {
    int index;
    const char *key;
    fylm::Kind kind;
    float low, high, value;
    int choices;
    const char *items;
};

// Index 0 is AE's implicit input layer; index 1..14 are the effect parameters.
constexpr Expected frozen[] = {
    {1, "effect.enabled", fylm::Kind::Check, 0, 1, 1, 0, nullptr},
    {2, "param.algorithm", fylm::Kind::Popup, 1, 6, 1, 6,
     "Floyd-Steinberg|Bayer 4x4|Bayer 8x8|Binary Threshold|Blue Noise 64x64|Cover Print Halftone"},
    {3, "param.palette", fylm::Kind::Popup, 1, 8, 1, 8,
     "--|Black/White|Game Boy|PICO-8|DawnBringer 16|DawnBringer 32|CGA|EGA"},
    {4, "param.threshold", fylm::Kind::Integer, 0, 255, 128, 0, nullptr},
    {5, "param.resolution", fylm::Kind::Integer, 0, 8192, 512, 0, nullptr},
    {6, "param.invert", fylm::Kind::Check, 0, 1, 0, 0, nullptr},
    {7, "param.serpentine", fylm::Kind::Check, 0, 1, 1, 0, nullptr},
    {8, "param.contrast", fylm::Kind::Float, -100, 100, 0, 0, nullptr},
    {9, "param.midtones", fylm::Kind::Float, .5f, 2, 1, 0, nullptr},
    {10, "param.highlights", fylm::Kind::Float, 0, 100, 0, 0, nullptr},
    {11, "param.alpha", fylm::Kind::Popup, 1, 2, 2, 2, "Opaque|Preserve source alpha"},
    {12, "halftone.bake", fylm::Kind::Integer, 8, 2048, 96, 0, nullptr},
    {13, "halftone.pitch", fylm::Kind::Float, 1, 16, 2.5f, 0, nullptr},
    {14, "color.linear_srgb", fylm::Kind::Check, 0, 1, 0, 0, nullptr},
};
constexpr int frozenCount = int(sizeof(frozen) / sizeof(frozen[0]));
constexpr Expected pixelFrozen[] = {
 {15,"pa.block",fylm::Kind::Integer,1,64,3,0,nullptr},
 {16,"pa.matrix",fylm::Kind::Popup,1,4,3,4,"None|Bayer 2x2|Bayer 4x4|Bayer 8x8"},
 {17,"pa.strength",fylm::Kind::Float,0,100,65,0,nullptr},{18,"pa.edge",fylm::Kind::Float,0,100,80,0,nullptr},
 {19,"pa.palette",fylm::Kind::Popup,1,3,1,3,"FYLM PC-98|Imported palette|Fixed reference frame"},
 {20,"pa.reference",fylm::Kind::Float,0,36000,0,0,nullptr},{21,"pa.colors",fylm::Kind::Integer,2,16,16,0,nullptr},
 {22,"pa.origin_x",fylm::Kind::Integer,-32768,32768,0,0,nullptr},{23,"pa.origin_y",fylm::Kind::Integer,-32768,32768,0,0,nullptr},
 {24,"pa.encoding",fylm::Kind::Popup,1,2,1,2,"sRGB encoded|Linear sRGB"},{25,"pa.alpha",fylm::Kind::Popup,1,2,1,2,"Coverage|Binary threshold"},
 {26,"pa.alpha_threshold",fylm::Kind::Float,0,1,.5f,0,nullptr},{27,"pa.data",fylm::Kind::Blob,0,0,0,0,nullptr},
 {28,"pa.import",fylm::Kind::Button,0,0,0,0,nullptr},{29,"pa.preset",fylm::Kind::Button,0,0,0,0,nullptr},{30,"pa.export",fylm::Kind::Button,0,0,0,0,nullptr},
 {31,"gba.block",fylm::Kind::Integer,1,64,6,0,nullptr},
 {32,"gba.colors",fylm::Kind::Integer,2,64,48,0,nullptr},
 {33,"gba.sampling",fylm::Kind::Popup,1,3,3,3,"Nearest|Area average|Feature representative"},
 {34,"gba.feature",fylm::Kind::Float,0,100,65,0,nullptr},
 {35,"gba.outline",fylm::Kind::Float,0,100,15,0,nullptr},
 {36,"modern.block",fylm::Kind::Integer,1,64,3,0,nullptr},
 {37,"modern.colors",fylm::Kind::Integer,2,128,96,0,nullptr},
 {38,"modern.sampling",fylm::Kind::Popup,1,3,3,3,"Nearest|Area average|Feature representative"},
 {39,"modern.merge",fylm::Kind::Float,0,100,35,0,nullptr},
 {40,"modern.cluster_area",fylm::Kind::Integer,0,32,4,0,nullptr},
 {41,"modern.speckles",fylm::Kind::Check,0,1,1,0,nullptr},
 {42,"modern.spurs",fylm::Kind::Check,0,1,1,0,nullptr},
 {43,"modern.stairs",fylm::Kind::Check,0,1,0,0,nullptr},
 {44,"modern.iterations",fylm::Kind::Integer,1,3,2,0,nullptr},
 {45,"modern.protection",fylm::Kind::Float,0,100,80,0,nullptr},
 {46,"modern.debug",fylm::Kind::Popup,1,2,1,2,"Final image|Changed pixels mask"},
 {47,"modern.outline",fylm::Kind::Float,0,100,0,0,nullptr},
 {48,"modern.feature",fylm::Kind::Float,0,100,50,0,nullptr},
 {49,"library.recipe",fylm::Kind::Popup,1,9,1,9,"Select a style|PA02 Soft Desktop 98|PA03 Midnight 98|PA04 Sunny Cartridge|PA06 Orbital Cartridge|PA07 Quiet Town|PA08 Crystal Journey|PA09 Neon Afterhours|PA12 Two-Tone Chronicle"},
 {50,"library.variant",fylm::Kind::Popup,1,2,1,2,"A|B"},
 {51,"library.apply",fylm::Kind::Button,0,0,0,0,nullptr},
 {52,"library.pc98.preset",fylm::Kind::Popup,1,7,1,7,"Select a preset|PA02 A|PA02 B|PA03 A|PA03 B|PA12 A|PA12 B"},
 {53,"library.pc98.apply",fylm::Kind::Button,0,0,0,0,nullptr},
 {54,"library.gba.preset",fylm::Kind::Popup,1,5,1,5,"Select a preset|PA04 A|PA04 B|PA06 A|PA06 B"},
 {55,"library.gba.apply",fylm::Kind::Button,0,0,0,0,nullptr},
 {56,"library.modern.preset",fylm::Kind::Popup,1,7,1,7,"Select a preset|PA07 A|PA07 B|PA08 A|PA08 B|PA09 A|PA09 B"},
 {57,"library.modern.apply",fylm::Kind::Button,0,0,0,0,nullptr}
};

void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool same(const char *a, const char *b) {
    if (!a || !b)
        return a == b;
    return std::strcmp(a, b) == 0;
}
} // namespace

int main() {
    try {
        // User-visible name may change; identity may not.
        require(same(fylm::Name, "FYLM Pixel Art"), "display name must be FYLM Pixel Art");
        require(same(fylm::Match, "FYLM_DitheringStudio"), "match name is frozen for old projects");
        require(same(fylm::Category, "FYLM Plugins"), "suite category is frozen");
        require(fylm::Count >= frozenCount + 1, "historical parameters removed");
        for (int i = 1; i <= frozenCount; ++i) {
            const auto &actual = fylm::Params[i - 1];
            const auto &want = frozen[i - 1];
            require(actual.kind == want.kind, "parameter type changed");
            require(same(actual.key, want.key), "parameter key or order changed");
            require(actual.low == want.low && ((i==2||i==3)?actual.high>=want.high:actual.high == want.high), "parameter range changed");
            require(actual.value == want.value, "parameter default changed");
            require((i==2||i==3)?actual.choices>=want.choices:actual.choices == want.choices, "popup choice count changed");
            require((i==2||i==3)?(std::strncmp(actual.items,want.items,std::strlen(want.items))==0&&(actual.items[std::strlen(want.items)]==0||actual.items[std::strlen(want.items)]=='|')):same(actual.items, want.items), "historical popup labels changed");
            require(fylm::index(want.key) == i, "key lookup does not match the frozen index");
        }
        // Appended parameters must start at 15 and keep their own ids; nothing may shift 1-14.
        require(fylm::index("param.algorithm") == 2, "algorithm must stay at index 2");
        for(auto want:pixelFrozen){require(want.index<fylm::Count,"PC98 control missing");auto actual=fylm::Params[want.index-1];require(fylm::index(want.key)==want.index&&actual.kind==want.kind&&actual.low==want.low&&((want.index==52||want.index==54||want.index==56)?actual.high>=want.high:actual.high==want.high)&&actual.value==want.value&&((want.index==52||want.index==54||want.index==56)?actual.choices>=want.choices:actual.choices==want.choices)&&((want.index==52||want.index==54||want.index==56)?(std::strncmp(actual.items,want.items,std::strlen(want.items))==0&&(actual.items[std::strlen(want.items)]==0||actual.items[std::strlen(want.items)]=='|')):same(actual.items,want.items)),"accepted PC98 parameter changed");}
        require(fylm::index("retro.mode") == -1 && fylm::index("sg.preset") == -1,
                "product-specific parameters leaked into the dither product");
        require(fylm::index("pa.spec")==58&&fylm::Params[57].value==1,"hardware spec must append with legacy-compatible default");
        require(fylm::index("pa.indexed_colors")==59&&fylm::Params[58].low==2&&fylm::Params[58].high==256&&fylm::Params[58].value==256,"indexed count range");
        require(fylm::index("pa.dac")==60&&fylm::Params[59].value==2,"24-bit programmable palette default");
        std::cout << fylm::Name << ": " << frozenCount
                  << " parameter identities, match name and category are frozen.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "parameter identity: " << e.what() << '\n';
        return 1;
    }
}
