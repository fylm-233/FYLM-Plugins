#include "pixel_art/core.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pa=pixel_art;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
unsigned code(pa::Rgb8 c){return (unsigned(c.r)<<16)|(unsigned(c.g)<<8)|c.b;}
int main(){try{
 using S=pa::PC98ColorSpec;const S specs[]={S::Digital8,S::Analog16,S::Indexed256,S::RGB565,S::RGB888};
 auto digital=pa::pc98HardwarePalette(S::Digital8);check(digital.size()==8,"3-bit capacity");for(auto c:digital)check((c.r==0||c.r==255)&&(c.g==0||c.g==255)&&(c.b==0||c.b==255),"digital RGB endpoints");
 auto analog=pa::pc98HardwarePalette(S::Analog16);check(analog.size()==16,"4-bit indexed capacity");for(auto c:analog)check(c.r%17==0&&c.g%17==0&&c.b%17==0,"RGB444 DAC");
 for(int bits:{6,8}){auto palette=pa::pc98HardwarePalette(S::Indexed256,256,bits);std::set<unsigned> unique;for(auto c:palette)unique.insert(code(c));check(unique.size()==256,"complete 256-colour default, not a short legacy palette");}
 auto dac6=pa::pc98ConformPalette({{123,45,67},{201,87,55}},S::Indexed256,6),dac8=pa::pc98ConformPalette({{123,45,67},{201,87,55}},S::Indexed256,8);check(code(dac6[0])!=code(dac8[0]),"DAC6 actually constrains arbitrary programmable palette entries");std::set<unsigned> levels6;for(int i=0;i<64;i++)levels6.insert(pa::toU8(i*255./63));for(auto c:dac6)check(levels6.count(c.r)&&levels6.count(c.g)&&levels6.count(c.b),"DAC6 channel grid");
 bool rejected=false;try{pa::pc98HardwarePalette(S::Indexed256,257);}catch(...){rejected=true;}check(rejected,"257 palette entries rejected");
 std::set<unsigned> all565;for(int r=0;r<32;r++)for(int g=0;g<64;g++)for(int b=0;b<32;b++)all565.insert(code(pa::pc98DirectColor(r/31.,g/63.,b/31.,S::RGB565)));check(all565.size()==65536,"all RGB565 colours addressable without palette allocation");
 check(pa::pc98HardwarePalette(S::RGB565).empty()&&pa::pc98HardwarePalette(S::RGB888).empty(),"direct colour must not enumerate palettes");
 for(int n=0;n<256;n++){auto c=pa::pc98DirectColor(n/255.,((n*7)%256)/255.,((n*13)%256)/255.,S::RGB888,.5);check(c.r==n&&c.g==(n*7)%256&&c.b==(n*13)%256,"RGB888 preserves encoded values; ordered dithering ignored");}
 constexpr int w=96,h=64;std::vector<unsigned char> input(w*h*4,255);for(int y=0;y<h;y++)for(int x=0;x<w;x++){auto i=(y*w+x)*4;input[i]=(x*19+y*7)%256;input[i+1]=(x*3+y*29)%256;input[i+2]=(x*31+y*11)%256;}
 for(auto spec:specs)for(auto encoding:{pa::ColorEncoding::Srgb,pa::ColorEncoding::LinearSrgb}){
  pa::Params p;p.pc98=true;p.pc98ColorSpec=spec;p.pc98PaletteBits=6;p.grid.block=3;p.grid.pixelAspect=1.25;p.grid.originX=-3;p.grid.downsampleX=p.grid.downsampleY=.5;p.encoding=encoding;
  std::vector<unsigned char> stream(input.size()),alias=input;check(pa::render({input.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},{stream.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},p).ok,"spec streaming render");check(pa::render({alias.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},{alias.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},p).ok&&stream==alias,"direct/indexed alias-safe storage, preview, PAR and phase parity");
  p.grid={};p.grid.block=1;p.ditherMatrix=0;
  for(auto format:{pa::PixelFormat::RGBA8,pa::PixelFormat::RGBA16,pa::PixelFormat::RGBA32F}){int size=format==pa::PixelFormat::RGBA8?4:format==pa::PixelFormat::RGBA16?8:16;std::vector<unsigned char> output(std::size_t(w)*h*size);check(pa::render({input.data(),w,h,w*4,pa::PixelFormat::RGBA8},{output.data(),w,h,std::size_t(w)*size,format},p).ok,"all colour specs at three output precisions");
   std::set<unsigned> colors;auto palette=pa::pc98HardwarePalette(spec,256,6);std::set<unsigned> allowed;for(auto c:palette)allowed.insert(code(c));
   for(int i=0;i<w*h;i++){unsigned rgb=0;for(int k=0;k<3;k++){double v;if(size==4)v=output[i*size+k]/255.;else if(size==8){std::uint16_t n;std::memcpy(&n,output.data()+i*size+k*2,2);v=n/65535.;}else{float n;std::memcpy(&n,output.data()+i*size+k*4,4);check(std::isfinite(n),"finite float direct output");v=n;}if(encoding==pa::ColorEncoding::LinearSrgb)v=pa::linearToSrgb(v);rgb=(rgb<<8)|pa::toU8(v*255);}
    colors.insert(rgb);if(encoding==pa::ColorEncoding::Srgb||size==16){if(spec==S::RGB565)check(all565.count(rgb)!=0,"RGB565 output set");else if(spec!=S::RGB888)check(allowed.count(rgb)!=0,"indexed output set at selected DAC precision");}
   }if(spec==S::Digital8)check(colors.size()<=8,"3-bit output count");if(spec==S::Analog16)check(colors.size()<=16,"4-bit output count");if(spec==S::Indexed256)check(colors.size()<=256,"8-bit output count");
  }
  // Alpha is independent of colour capacity, including hidden RGB and binary mode.
  auto soft=input;for(int i=0;i<w*h;i++)soft[i*4+3]=static_cast<unsigned char>(i%3==0?0:i%3==1?64:255);p.encoding=pa::ColorEncoding::Srgb;std::vector<unsigned char> output(soft.size());check(pa::render({soft.data(),w,h,w*4,pa::PixelFormat::RGBA8},{output.data(),w,h,w*4,pa::PixelFormat::RGBA8},p).ok,"soft alpha");for(int i=0;i<w*h;i++){check(output[i*4+3]==soft[i*4+3],"coverage independent of RGB bit depth");if(!output[i*4+3])check(output[i*4]==0&&output[i*4+1]==0&&output[i*4+2]==0,"zero alpha clears hidden RGB");}p.binaryAlpha=true;check(pa::render({soft.data(),w,h,w*4,pa::PixelFormat::RGBA8},{output.data(),w,h,w*4,pa::PixelFormat::RGBA8},p).ok,"binary alpha");for(int i=0;i<w*h;i++)check(output[i*4+3]==0||output[i*4+3]==255,"binary alpha endpoints");
 }
 // Complete indexed source must round-trip every one of its 256 entries.
 auto pal=pa::pc98HardwarePalette(S::Indexed256);std::vector<unsigned char> swatches(256*4,255),mapped(swatches.size());for(int i=0;i<256;i++){swatches[i*4]=pal[i].r;swatches[i*4+1]=pal[i].g;swatches[i*4+2]=pal[i].b;}pa::Params p;p.pc98=true;p.pc98ColorSpec=S::Indexed256;p.grid.block=1;p.ditherMatrix=0;check(pa::render({swatches.data(),256,1,1024,pa::PixelFormat::RGBA8},{mapped.data(),256,1,1024,pa::PixelFormat::RGBA8},p).ok&&swatches==mapped,"all 256 indices round-trip");
 auto extracted=pa::extractPalette({input.data(),w,h,w*4,pa::PixelFormat::RGBA8},pa::ColorEncoding::Srgb,256);check(extracted.size()==256,"reference extraction supports full 8-bit indexed palette");
 // Auto parallelism must remain deterministic for new paths on the available machine.
 constexpr int bw=512,bh=512;std::vector<unsigned char> large(bw*bh*4,255),serial(large.size()),parallel(large.size());for(std::size_t i=0;i<large.size();i++)large[i]=input[i%input.size()];
 for(auto spec:specs){p={};p.pc98=true;p.pc98ColorSpec=spec;p.grid.block=4;p.workers=1;check(pa::render({large.data(),bw,bh,bw*4,pa::PixelFormat::RGBA8},{serial.data(),bw,bh,bw*4,pa::PixelFormat::RGBA8},p).ok,"spec serial");p.workers=0;check(pa::render({large.data(),bw,bh,bw*4,pa::PixelFormat::RGBA8},{parallel.data(),bw,bh,bw*4,pa::PixelFormat::RGBA8},p).ok&&parallel==serial,"spec auto-worker parity");}
 constexpr int wide=3840,tall=2160;large.assign(std::size_t(wide)*tall*4,255);serial.resize(large.size());p={};p.pc98=true;p.pc98ColorSpec=S::RGB888;p.grid.block=1;p.memoryBudget=1024*1024;check(pa::render({large.data(),wide,tall,wide*4,pa::PixelFormat::RGBA8},{serial.data(),wide,tall,wide*4,pa::PixelFormat::RGBA8},p).ok&&large==serial,"4K true colour within 1 MiB core workspace, no giant palette");
 p.pc98ColorSpec=static_cast<S>(99);check(!pa::render({input.data(),w,h,w*4,pa::PixelFormat::RGBA8},{mapped.data(),1,1,4,pa::PixelFormat::RGBA8},p).ok,"invalid colour model rejected");
 std::cout<<"PC98 specs: 3/4/8-bit indexed, 18/24-bit DAC, exhaustive RGB565, RGB888, three output precisions, alpha, ROI, preview, alias, auto workers and bounded 4K passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
