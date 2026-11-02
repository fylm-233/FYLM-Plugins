#pragma once
#include "core.hpp"
namespace dither {
// Reimu cover-print halftone. Straight, encoded-sRGB RGBA8; alpha stays byte-exact.
// Row-major feedback and immediate half-away-from-zero rounding are intentional.
RenderResult applyHalftone(std::vector<std::uint8_t>& rgba,int width,int height,
                          float pitch=2.5f,IProgress* progress=nullptr) noexcept;
}
