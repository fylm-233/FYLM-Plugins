#pragma once
#include "core.hpp"
#include <map>
#include <string>
namespace pixel_art {
struct PaletteVariant {std::string id,name,variant,hash;std::vector<Rgb8> colors;};
struct ArtPreset {std::string id,name;int revision=1,mode=7,logicalWidth=640;std::map<std::string,double> settings;std::vector<PaletteVariant> variants;};
const std::vector<ArtPreset>& artPresets();
const ArtPreset& artPreset(const std::string& id);
std::map<std::string,double> artPresetValues(const ArtPreset&,int variant,int fullLayerWidth);
Params artPresetParams(const ArtPreset&,int variant,int fullLayerWidth);
// Complete palette and provenance, stored in AE arbitrary data by the UI action.
std::vector<unsigned char> artPresetPalette(const ArtPreset&,int variant,int fullLayerWidth);
}
