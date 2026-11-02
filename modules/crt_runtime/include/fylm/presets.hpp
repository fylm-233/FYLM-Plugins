#pragma once
#include "preset_catalog.hpp"
#include <map>
#include <string>
#include <vector>
namespace fylm::crt {
struct VhsPreset {std::string id,code,name;int choice=0;std::map<std::string,double> settings;};
const std::vector<VhsPreset>& vhsPresets();
const VhsPreset& vhsPreset(int choice);
}
