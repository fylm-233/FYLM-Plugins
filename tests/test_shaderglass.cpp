#include "preset.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <algorithm>
int main(int argc,char** argv){try{if(argc!=2)return 2;auto preset=fylm::sg::load(argv[1]);retro::RenderRequest r;r.size={96,64};r.timeScale=24;r.input.resize(96*64*4);
 for(int y=0;y<64;++y)for(int x=0;x<96;++x){int i=(y*96+x)*4;r.input[i]=x/95.f;r.input[i+1]=y/63.f;r.input[i+2]=.5f;r.input[i+3]=1;}
 fylm::sg::Settings s;s.virtualHeight=preset->controls.empty()?0:32;auto a=fylm::sg::render(*preset,r,s);if(!a.ok)throw std::runtime_error(a.error);auto b=fylm::sg::render(*preset,r,s);if(!b.ok||a.pixels!=b.pixels)throw std::runtime_error("repeat differs");
 float sum=0;for(float v:a.pixels){if(!std::isfinite(v))throw std::runtime_error("nonfinite output");sum+=v;}if(sum<100)throw std::runtime_error("blank output");
 if(preset->controls.empty()){for(std::size_t i=0;i<a.pixels.size();++i)if(std::abs(a.pixels[i]-r.input[i])>1e-5f)throw std::runtime_error("Multipass identity/orientation mismatch");}
 else{if(a.pixels==r.input)throw std::runtime_error("preset did not apply");s.slots=true;s.values[0]=80;auto changed=fylm::sg::render(*preset,r,s);if(!changed.ok||changed.pixels==a.pixels)throw std::runtime_error("Parameter override ineffective");}
 auto output=std::filesystem::path(argv[1]);output.replace_extension(".test.rgba");std::ofstream file(output,std::ios::binary);for(float v:a.pixels){unsigned char q=static_cast<unsigned char>(std::round(std::clamp(v,0.f,1.f)*255));file.write(reinterpret_cast<char*>(&q),1);}
 std::cout<<preset->name<<": actual slang compilation/load/D3D11 render passed, "<<preset->passes.size()<<" passes, "<<preset->controls.size()<<" parameters\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
