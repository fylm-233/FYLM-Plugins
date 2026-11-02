#include "fylm/presets.hpp"
#include "json.hpp"
#include <cmath>
#include <set>
namespace fylm::crt {
const std::vector<VhsPreset>& vhsPresets(){static const auto presets=[](){
 auto data=nlohmann::json::parse(VhsPanelJson);if(data.size()!=8)throw std::runtime_error("retro.error.preset_guard");std::vector<VhsPreset> result;std::set<std::string> ids;
 for(auto& e:data){VhsPreset p;p.id=e.at("id");p.code=e.at("code");p.name=e.at("name");p.choice=e.at("choice");p.settings=e.at("settings").get<std::map<std::string,double>>();if(p.choice!=int(result.size())+2||!ids.insert(p.id).second||p.settings.empty())throw std::runtime_error("retro.error.preset_guard");for(auto& [key,value]:p.settings)if(!std::isfinite(value))throw std::runtime_error("retro.error.preset_guard");result.push_back(std::move(p));}return result;
 }();return presets;}
const VhsPreset& vhsPreset(int choice){if(choice<2||choice>9)throw std::runtime_error("retro.error.preset_guard");return vhsPresets().at(choice-2);}
}
