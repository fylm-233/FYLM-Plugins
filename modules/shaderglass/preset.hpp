#pragma once
#include "retro/renderer.hpp"
#include <filesystem>
#include <map>
#include <memory>
namespace fylm::sg {
struct Uniform {std::string name,description;int buffer=0,offset=0,size=0;float low=0,high=1,value=0;};
struct Sampler {std::string name;int binding=0;};
struct Pass {std::vector<unsigned char> vertex,fragment;std::vector<Uniform> uniforms;std::vector<Sampler> samplers;std::map<std::string,std::string> settings;std::string format;};
struct Resource {std::string name,hash;std::vector<unsigned char> encoded,rgba;unsigned width=0,height=0;std::map<std::string,std::string> settings;};
struct Preset {std::string name,id;float mix=1;std::vector<Pass> passes;std::vector<Uniform> controls;std::vector<Resource> resources;std::vector<std::string> slots;};
std::string digest(const std::vector<unsigned char>&);
void decodeResource(Resource&);
bool supportedFormat(const std::string&);
std::string describe(const Preset&);
std::string scan(const std::filesystem::path&);
const Uniform* slotControl(const Preset&,std::size_t);
// Validate execution dependencies before replacing a working host preset.
void requireRuntimeSupport(const Preset&);
Preset compile(const std::filesystem::path&);
void save(const Preset&,const std::filesystem::path&);
std::vector<unsigned char> encode(const Preset&);
std::shared_ptr<const Preset> load(const std::filesystem::path&);
std::shared_ptr<const Preset> loadBytes(const std::vector<unsigned char>&);
struct Settings {int virtualHeight=240,frameIndex=0,frameDirection=1;bool slots=false,extendedSlots=false;float timeSeconds=0;float mix=1;float values[16]={50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50};float pixelAspect=1,frameRate=24;std::map<std::string,float> namedValues;};
std::string rendererBackend();
std::uint64_t rendererPeakBytes();
std::uint64_t rendererBudgetBytes();
retro::RenderOutput render(const Preset&,const retro::RenderRequest&,const Settings&);
}
