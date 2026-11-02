#pragma once
#include "retro.hpp"
#include <functional>
namespace retro {
// Caller owns current GL context and exclusive device lease. All GL objects are call-private.
// Pixels are top-row-first RGBA float in host-value color space/alpha association.
// No implicit gamma conversion or premultiplication conversion is performed.
struct RenderRequest {
    Controls controls;
    Size size;
    std::int64_t timeValue=0;
    std::uint64_t timeScale=1;
    std::vector<float> input;
    std::vector<float> palette; // RGB triples, same domain as input
    std::function<bool()> cancelled;
    // Used only for FS: preserves existing CPU diffusion/tone/resize/alpha semantics.
    std::function<std::vector<float>(const std::vector<float>&)> cpuDither;
    std::size_t memoryBudget=512u*1024u*1024u;
};
struct RenderOutput { bool ok=false;std::string error;std::vector<float> pixels; };
RenderOutput renderOpenGL(const RenderRequest&) noexcept;
}
