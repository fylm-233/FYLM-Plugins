#include "library.hpp"
#include "sg_library_data.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
namespace fylm::sg {
namespace {
const nlohmann::json& catalog(){static const auto j=nlohmann::json::parse(data::catalog);return j;}
const nlohmann::json& entry(int selection){
 if(selection<1||selection>LibraryMenuCount||selection==3)throw std::runtime_error("sg.error.schema");
 return catalog().at("entries").at(selection-1);
}
}
std::vector<unsigned char> builtinBytes(int selection){
 auto name=entry(selection).at("base").get<std::string>();
 for(auto& blob:data::blobs)if(name==blob.name)return {blob.bytes,blob.bytes+blob.size};
 throw std::runtime_error("sg.error.read");
}
std::shared_ptr<const Preset> builtinPreset(int selection){
 static std::mutex mutex;static std::map<int,std::shared_ptr<const Preset>> cache;
 std::lock_guard<std::mutex> lock(mutex);if(auto it=cache.find(selection);it!=cache.end())return it->second;
 const auto& e=entry(selection);auto original=loadBytes(builtinBytes(selection));
 // Frozen entries retain their original parameter-slot order and all original defaults.
 if(selection<=2){requireRuntimeSupport(*original);cache[selection]=original;return original;}
 auto p=std::make_shared<Preset>(*original);p->name=e.at("name");p->mix=e.value("mix",1.f);p->slots=e.at("slots").get<std::vector<std::string>>();
 if(p->slots.size()>16)throw std::runtime_error("sg.error.uniform");
 for(const auto& name:p->slots)if(std::none_of(p->controls.begin(),p->controls.end(),[&](const auto& u){return u.name==name;}))throw std::runtime_error("sg.error.uniform");
 for(auto it=e.at("overrides").begin();it!=e.at("overrides").end();++it){
  auto c=std::find_if(p->controls.begin(),p->controls.end(),[&](const auto& u){return u.name==it.key();});float value=it.value();
  if(c==p->controls.end()||!std::isfinite(value)||value<c->low||value>c->high)throw std::runtime_error("sg.error.uniform");
  c->value=value;for(auto& pass:p->passes)for(auto& u:pass.uniforms)if(u.name==it.key())u.value=value;
 }
 // Program/texture caches must distinguish recipes sharing the same source bytecode.
 auto identity=original->id+e.dump();p->id="sha256:"+digest({identity.begin(),identity.end()});
 requireRuntimeSupport(*p);cache[selection]=p;return p;
}
int recommendedHeight(int selection,int manualHeight,bool follow){
 // Existing Geom/EasyMode and Imported continue to honour their saved height.
 if(!follow||selection<=3)return manualHeight;
 return entry(selection).at("virtual_height").get<int>();
}
}
