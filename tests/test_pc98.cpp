#include "pixel_art/core.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pa=pixel_art;
void check(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
int main(){try{
 const int w=64,h=32;std::vector<unsigned char> input(w*h*4,255),a(input.size()),b(input.size()),baseline(input.size());
 for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<3;c++)input[(y*w+x)*4+c]=static_cast<unsigned char>(x*255/(w-1));
 pa::Params p;p.palette=pa::pc98Palette();p.grid.block=2;p.pc98=true;p.edgeProtection=0;p.ditherStrength=1;
 auto in=pa::ConstImageView{input.data(),w,h,std::size_t(w)*4,pa::PixelFormat::RGBA8};auto out=[&](auto& v){return pa::ImageView{v.data(),w,h,std::size_t(w)*4,pa::PixelFormat::RGBA8};};
 check(pa::render(in,out(a),p).ok,"PC98 render");check(pa::render(in,out(b),p).ok&&a==b,"repeat");
 std::set<int> colors;for(auto c:p.palette)colors.insert((c.r<<16)|(c.g<<8)|c.b);for(std::size_t i=0;i<a.size();i+=4)check(colors.count((a[i]<<16)|(a[i+1]<<8)|a[i+2])!=0,"palette invariant");
 p.ditherStrength=0;check(pa::render(in,out(baseline),p).ok,"zero strength");p.pc98=false;check(pa::render(in,out(b),p).ok&&b==baseline,"zero equals nearest");check(a!=baseline,"dither has effect");
 // Destination cropping must use full input and retain identical grid phase.
 p.pc98=true;p.ditherStrength=1;std::vector<unsigned char> crop(17*11*4);check(pa::render(in,{crop.data(),17,11,17*4,pa::PixelFormat::RGBA8,13,7},p).ok,"ROI render");
 for(int y=0;y<11;y++)for(int x=0;x<17;x++)for(int c=0;c<4;c++)check(crop[(y*17+x)*4+c]==a[((y+7)*w+x+13)*4+c],"ROI phase");
 // Uniform input isolates phase from differences in the host's source resampling.
 std::vector<unsigned char> uniform(input.size(),128),full(input.size()),half(32*16*4),reduced(32*16*4,128);for(std::size_t i=3;i<uniform.size();i+=4)uniform[i]=255;for(std::size_t i=3;i<reduced.size();i+=4)reduced[i]=255;
 p.grid.block=4;check(pa::render({uniform.data(),w,h,std::size_t(w)*4,pa::PixelFormat::RGBA8},out(full),p).ok,"full phase baseline");p.grid.downsampleX=p.grid.downsampleY=.5;
 check(pa::render({reduced.data(),32,16,32*4,pa::PixelFormat::RGBA8},{half.data(),32,16,32*4,pa::PixelFormat::RGBA8},p).ok,"half phase render");for(int y=0;y<16;y++)for(int x=0;x<32;x++)for(int c=0;c<4;c++)check(half[(y*32+x)*4+c]==full[(((2*y+1)*w)+(2*x+1))*4+c],"half grid phase");p.grid.downsampleX=p.grid.downsampleY=1;
 auto first=pa::extractPalette(in,pa::ColorEncoding::Srgb,16);check(first.size()<=16&&!first.empty(),"extract count");auto second=pa::extractPalette(in,pa::ColorEncoding::Srgb,16);check(first.size()==second.size(),"extract deterministic count");for(std::size_t i=0;i<first.size();i++)check(first[i].r==second[i].r&&first[i].g==second[i].g&&first[i].b==second[i].b,"extract deterministic");
 std::vector<float> floats(w*h*4,.5f),result(floats.size());for(std::size_t i=3;i<floats.size();i+=4)floats[i]=1;floats[0]=INFINITY;floats[1]=-2;
 check(pa::render({floats.data(),w,h,std::size_t(w)*16,pa::PixelFormat::RGBA32F},{result.data(),w,h,std::size_t(w)*16,pa::PixelFormat::RGBA32F},p).ok,"float SDR mapping");for(float v:result)check(std::isfinite(v)&&v>=0&&v<=1,"float finite");
 std::fill(input.begin(),input.end(),0);p.binaryAlpha=true;p.alphaThreshold=0;check(pa::render(in,out(a),p).ok&&std::all_of(a.begin(),a.end(),[](auto v){return v==0;}),"transparent binary threshold zero");
 std::cout<<"PC98 palette/ordered-dither/zero strength, ROI, reference extraction, float SDR and alpha passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
