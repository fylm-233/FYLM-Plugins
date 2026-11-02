#pragma once
#include "core.hpp"
namespace pixel_art {
// All edits select existing palette labels. Inputs/outputs are logical-grid data.
// Returned mask compares final labels with input; refinement never edits alpha.
std::vector<unsigned char> refineLabels(std::vector<unsigned short>& labels,int width,int height,
 const std::vector<Lab>& palette,const std::vector<float>& alpha,const Params&,IProgress* progress=nullptr);
void outlineLabels(std::vector<unsigned short>& labels,int width,int height,
 const std::vector<Lab>& palette,const std::vector<float>& alpha,double strength);
}
