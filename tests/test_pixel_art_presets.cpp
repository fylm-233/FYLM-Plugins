#include "pixel_art/presets.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pa=pixel_art;
void check(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
int main(){try{
 const char* ids[]={"PA02","PA03","PA04","PA06","PA07","PA08","PA09","PA12","PA01","PA05","PA10","PA11"};const int counts[]={16,16,48,48,96,64,64,2,4,64,64,64};check(pa::artPresets().size()==12,"first library size");
 constexpr int w=96,h=64;std::vector<unsigned char> input(w*h*4,255);for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<3;c++)input[(y*w+x)*4+c]=static_cast<unsigned char>((x*19+y*7+c*41)%256);
 for(int i=0;i<12;i++){auto& preset=pa::artPresets()[i];check(preset.id==ids[i]&&&pa::artPreset(ids[i])==&preset,"stable ordered identities");check(preset.variants[0].hash!=preset.variants[1].hash,"A/B distinct palettes");
  for(int v=0;v<2;v++){auto& palette=preset.variants[v];check(palette.colors.size()==std::size_t(counts[i]),"complete target colour count");std::set<unsigned> colors;for(auto c:palette.colors)colors.insert((c.r<<16)|(c.g<<8)|c.b);check(colors.size()==palette.colors.size(),"unique colour entries");auto values=pa::artPresetValues(preset,v,1920);check(values.at("param.algorithm")==preset.mode&&values.at("pa.palette")==2,"host recipe strategy");for(auto key:{"pa.alpha","pa.alpha_threshold","pa.encoding","pa.origin_x","pa.origin_y","effect.enabled"})check(!values.count(key),"recipe must preserve colour/alpha/grid origin and enable state");
   auto params=pa::artPresetParams(preset,v,w);for(auto format:{pa::PixelFormat::RGBA8,pa::PixelFormat::RGBA16,pa::PixelFormat::RGBA32F}){int size=format==pa::PixelFormat::RGBA8?4:format==pa::PixelFormat::RGBA16?8:16;std::vector<unsigned char> output(std::size_t(w)*h*size),again(output.size());auto target=[&](auto& bytes){return pa::ImageView{bytes.data(),w,h,std::size_t(w)*size,format};};check(pa::render({input.data(),w,h,w*4,pa::PixelFormat::RGBA8},target(output),params).ok&&pa::render({input.data(),w,h,w*4,pa::PixelFormat::RGBA8},target(again),params).ok&&output==again,"all variants/depths deterministic render");
    for(int pixel=0;pixel<w*h;pixel++){unsigned rgb=0;for(int c=0;c<3;c++){unsigned code=0;auto p=output.data()+std::size_t(pixel)*size;if(size==4)code=p[c];else if(size==8){std::uint16_t value;std::memcpy(&value,p+c*2,2);check(value%257==0,"RGBA16 strict colour scale");code=value/257;}else{float value;std::memcpy(&value,p+c*4,4);code=unsigned(std::lround(value*255));check(std::abs(value-float(code/255.))<1e-7,"float strict colour scale");}rgb=(rgb<<8)|code;}check(colors.count(rgb)!=0,"raw opaque output remains in palette");}
   }
   auto bytes=pa::artPresetPalette(preset,v,2560);check(bytes.size()<65536&&!bytes.empty(),"complete embedded palette size");
   // Hidden colour may never turn an entirely transparent portrait into visible colour.
   auto hidden=input;for(int pixel=0;pixel<w*h;pixel++)hidden[pixel*4+3]=0;
   for(auto format:{pa::PixelFormat::RGBA8,pa::PixelFormat::RGBA16,pa::PixelFormat::RGBA32F}){int size=format==pa::PixelFormat::RGBA8?4:format==pa::PixelFormat::RGBA16?8:16;std::vector<unsigned char> output(std::size_t(w)*h*size,255);check(pa::render({hidden.data(),w,h,w*4,pa::PixelFormat::RGBA8},{output.data(),w,h,std::size_t(w)*size,format},params).ok,"transparent preset render");check(std::all_of(output.begin(),output.end(),[](unsigned char byte){return byte==0;}),"hidden RGB must not escape transparent input");}
  }
 }
 bool failed=false;try{pa::artPreset("unknown");}catch(...){failed=true;}check(failed,"unknown identity rejected");failed=false;try{pa::artPresetValues(pa::artPresets()[0],2,1920);}catch(...){failed=true;}check(failed,"invalid variant rejected");
 std::cout<<"12 recipes / 24 complete palettes, 72 depth cases, strict colours, identity, scope and determinism passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
