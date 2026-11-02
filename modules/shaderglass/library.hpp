#pragma once
#include "preset.hpp"
#include "library_catalog.hpp"
namespace fylm::sg {
// Selection 3 is reserved for the project's imported arbitrary-data payload.
std::shared_ptr<const Preset> builtinPreset(int selection);
int recommendedHeight(int selection, int manualHeight, bool follow);
std::vector<unsigned char> builtinBytes(int selection);
}
