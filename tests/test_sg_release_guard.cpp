#include "preset.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace fylm::sg;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{if(argc!=2)return 2;auto p=*load(argv[1]);requireRuntimeSupport(p);auto rejected=[&](Preset value){try{requireRuntimeSupport(value);return false;}catch(...){return true;}};
 auto resources=p;resources.resources.emplace_back();check(rejected(resources),"invalid resource accepted");auto mip=p;mip.passes[0].settings["mipmap_input"]="true";check(!rejected(mip),"mipmap capability unavailable");auto format=p;format.passes[0].format="R16G16B16A16_SFLOAT";check(!rejected(format),"half float format unavailable");format.passes[0].format="FAKE_FORMAT";check(rejected(format),"unknown format accepted");auto history=p;history.passes[0].samplers.push_back({"OriginalHistory1",15});check(rejected(history),"true history silently substituted");
 std::ifstream file(argv[1],std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};auto j=nlohmann::json::from_cbor(bytes);j["schema"]="fylm.shaderglass/1";j.erase("slots");j.erase("resources");for(auto& pass:j["passes"])pass.erase("format");auto old=loadBytes(nlohmann::json::to_cbor(j));requireRuntimeSupport(*old);check(old->passes.size()==p.passes.size()&&old->controls.size()==p.controls.size(),"legacy package schema rejected");check(slotControl(*old,0)&&slotControl(*old,0)->name==old->controls[0].name,"legacy slot order changed");
 if(p.controls.size()>1){p.slots={p.controls[1].name,p.controls[0].name};check(slotControl(p,0)->name==p.controls[1].name&&slotControl(p,1)->name==p.controls[0].name,"named slots ignored");}
 auto deep=nlohmann::json::array();for(int i=0;i<100;i++)deep=nlohmann::json::array({deep});bool bounded=false;try{loadBytes(nlohmann::json::to_cbor(deep));}catch(...){bounded=true;}check(bounded,"deep package accepted");
 std::cout<<"Release subset rejection, schema1 compatibility and name-bound slots passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
