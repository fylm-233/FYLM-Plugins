#include "pixel_art/presets.hpp"
#include "pixel_art_preset_data.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <stdexcept>
namespace pixel_art {
const std::vector<ArtPreset>& artPresets(){
 static const auto catalog=[](){auto j=nlohmann::json::parse(pixelArtCatalog);if(j.at("schema_version")!=1||j.at("encoding")!="srgb"||j.at("algorithm_version")!=1)throw std::runtime_error("preset schema");std::vector<ArtPreset> out;std::set<std::string> ids;
 for(auto& entry:j.at("presets")){ArtPreset preset;preset.id=entry.at("id");preset.name=entry.at("name");preset.revision=entry.at("revision");preset.mode=entry.at("mode");preset.logicalWidth=entry.at("logical_width");if(!ids.insert(preset.id).second||preset.revision!=1||preset.mode<7||preset.mode>9||preset.logicalWidth<1)throw std::runtime_error("preset identity");
  for(auto& item:entry.at("settings").items()){double value=item.value().get<double>();if(!std::isfinite(value))throw std::runtime_error("preset parameter");preset.settings[item.key()]=value;}
  for(auto& value:entry.at("variants")){PaletteVariant variant;variant.id=value.at("id");variant.name=value.at("name");variant.variant=value.at("variant");variant.hash=value.at("content_sha256");if(!ids.insert(variant.id).second||variant.hash.size()!=64)throw std::runtime_error("palette identity");std::set<unsigned> colors;
   for(auto& item:value.at("colors")){auto hex=item.get<std::string>();if(hex.size()!=7||hex[0]!='#'||hex.find_first_not_of("0123456789abcdefABCDEF",1)!=std::string::npos)throw std::runtime_error("palette hex");unsigned color=std::stoul(hex.substr(1),nullptr,16);if(!colors.insert(color).second)throw std::runtime_error("palette duplicate");variant.colors.push_back({static_cast<unsigned char>(color>>16),static_cast<unsigned char>(color>>8),static_cast<unsigned char>(color)});}
   auto count=variant.colors.size();if(count<2||count>std::size_t(preset.mode==7?16:preset.mode==8?64:128))throw std::runtime_error("palette limit");preset.variants.push_back(std::move(variant));
  }if(preset.variants.size()!=2||preset.variants[0].variant!="A"||preset.variants[1].variant!="B")throw std::runtime_error("palette variants");out.push_back(std::move(preset));
 }if(out.size()!=12)throw std::runtime_error("preset catalog count");return out;}();return catalog;
}
const ArtPreset& artPreset(const std::string& id){for(auto& preset:artPresets())if(preset.id==id)return preset;throw std::invalid_argument("Unknown FYLM Pixel Art preset");}
std::map<std::string,double> artPresetValues(const ArtPreset& preset,int variant,int width){
 if(variant<0||variant>=int(preset.variants.size())||width<1||width>32768)throw std::invalid_argument("Preset dimensions or variant");auto values=preset.settings;values["pa.palette"]=2;values[preset.mode==7?"pa.colors":preset.mode==8?"gba.colors":"modern.colors"]=double(preset.variants[variant].colors.size());values[preset.mode==7?"pa.block":preset.mode==8?"gba.block":"modern.block"]=std::clamp(int(std::lround(double(width)/preset.logicalWidth)),1,64);return values;
}
Params artPresetParams(const ArtPreset& preset,int variant,int width){auto values=artPresetValues(preset,variant,width);Params p;p.palette=preset.variants[variant].colors;p.pc98=preset.mode==7;p.style=preset.mode==7?Style::Basic:preset.mode==8?Style::GBA:Style::Modern;p.grid.block=int(values.at(preset.mode==7?"pa.block":preset.mode==8?"gba.block":"modern.block"));
 if(p.pc98){constexpr int matrices[]={0,2,4,8};p.ditherMatrix=matrices[int(values.at("pa.matrix"))-1];p.ditherStrength=values.at("pa.strength")/100;p.edgeProtection=values.at("pa.edge")/100;p.workers=0;}
 else {p.grid.sampling=Sampling::Feature;auto prefix=preset.mode==8?"gba.":"modern.";p.featureStrength=values.at(std::string(prefix)+"feature")/100;p.outlineStrength=values.at(std::string(prefix)+"outline")/100;if(preset.mode==9){p.mergeThreshold=values.at("modern.merge")*.0012;p.maxClusterArea=int(values.at("modern.cluster_area"));p.detailProtection=values.at("modern.protection")/100;p.cleanupIterations=int(values.at("modern.iterations"));p.cleanSpeckles=values.at("modern.speckles")!=0;p.cleanSpurs=values.at("modern.spurs")!=0;p.cleanStairs=values.at("modern.stairs")!=0;}}
 return p;
}
std::vector<unsigned char> artPresetPalette(const ArtPreset& preset,int variant,int width){auto values=artPresetValues(preset,variant,width);auto& palette=preset.variants[variant];nlohmann::json j={{"schema_version",1},{"encoding","srgb"},{"name",preset.name+" / "+palette.name},{"preset_id",preset.id},{"preset_revision",preset.revision},{"palette_id",palette.id},{"content_sha256",palette.hash},{"algorithm_version",1},{"applied_values",values},{"color_origin","FYLM original authored ramps, not official game colours"},{"colors",nlohmann::json::array()}};for(auto color:palette.colors){char text[8];std::snprintf(text,sizeof(text),"#%02X%02X%02X",color.r,color.g,color.b);j["colors"].push_back(text);}auto text=j.dump(2);return {text.begin(),text.end()};}
}
