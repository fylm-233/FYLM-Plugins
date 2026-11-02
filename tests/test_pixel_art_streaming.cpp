#include "pixel_art/core.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pa=pixel_art;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){try{
 // The full-grid, alias-safe fallback serves as an independent storage-path
 // reference. Exercise subpixel cells, preview phase, non-square pixels and alpha.
 constexpr int w=96,h=64;std::vector<unsigned char> source(w*h*4),stream(source.size());
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){auto i=(y*w+x)*4;source[i]=static_cast<unsigned char>((x*17+y*3)%256);source[i+1]=static_cast<unsigned char>((x*7+y*19)%256);source[i+2]=static_cast<unsigned char>((x*11+y*5)%256);source[i+3]=static_cast<unsigned char>((x+y)%4?255:64);}
 for(double preview:{1.,.5,.25})for(int block:{1,3,6})for(auto encoding:{pa::ColorEncoding::Srgb,pa::ColorEncoding::LinearSrgb}){
  pa::Params p;p.pc98=true;p.palette=pa::pc98Palette();p.grid.block=block;p.grid.downsampleX=p.grid.downsampleY=preview;p.grid.pixelAspect=1.25;p.grid.originX=-3;p.grid.originY=2;p.encoding=encoding;
  auto reference=source;check(pa::render({reference.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},{reference.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},p).ok,"full-grid reference");
  p.memoryBudget=1024*1024;check(pa::render({source.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},{stream.data(),w,h,w*4,pa::PixelFormat::RGBA8,-7,5},p).ok&&stream==reference,"streaming matches full graph at preview/PAR/origin/alpha");
 }
 // A 4K one-column grid previously failed the 256 MiB full-grid estimate.
 // Input/output storage belongs to the caller; core workspace is now <1 MiB.
 constexpr int bigW=3840,bigH=2160;std::vector<unsigned char> big(std::size_t(bigW)*bigH*4,255),result(big.size());pa::Params p;p.pc98=true;p.palette={{255,255,255}};p.grid.block=1;p.ditherMatrix=0;p.memoryBudget=1024*1024;
 check(pa::render({big.data(),bigW,bigH,bigW*4,pa::PixelFormat::RGBA8},{result.data(),bigW,bigH,bigW*4,pa::PixelFormat::RGBA8},p).ok&&result==big,"4K block one within 1 MiB workspace");
 p.palette=pa::pc98Palette();p.ditherMatrix=4;p.memoryBudget=2*1024*1024;unsigned char cropped[4]{};
 check(pa::render({big.data(),bigW,bigH,bigW*4,pa::PixelFormat::RGBA8},{cropped,1,1,4,pa::PixelFormat::RGBA8,2000,1000},p).ok,"4K active PC98/ROI within 2 MiB workspace");
 p.memoryBudget=16;check(pa::render({big.data(),bigW,bigH,bigW*4,pa::PixelFormat::RGBA8},{result.data(),bigW,bigH,bigW*4,pa::PixelFormat::RGBA8},p).code==pa::ErrorCode::OutOfMemory,"real budget still enforced");
 // Direct premultiplied ARGB writes must match the old float intermediate,
 // including 32768-scale integer conversion and untouched row padding.
 p={};p.pc98=true;p.palette=pa::pc98Palette();p.grid.block=3;std::vector<float> rgba(w*h*4);
 check(pa::render({source.data(),w,h,w*4,pa::PixelFormat::RGBA8},{rgba.data(),w,h,w*16,pa::PixelFormat::RGBA32F},p).ok,"float reference");
 for(auto format:{pa::PixelFormat::ARGB8Premultiplied,pa::PixelFormat::ARGB16_32768Premultiplied,pa::PixelFormat::ARGB32FPremultiplied}){
  int bytes=format==pa::PixelFormat::ARGB8Premultiplied?4:format==pa::PixelFormat::ARGB16_32768Premultiplied?8:16;int stride=w*bytes+32;std::vector<unsigned char> direct(stride*h,0xCD);
  check(pa::render({source.data(),w,h,w*4,pa::PixelFormat::RGBA8},{direct.data(),w,h,std::size_t(stride),format},p).ok,"direct ARGB render");
  for(int y=0;y<h;y++){for(int x=0;x<w;x++){auto i=(y*w+x)*4;float a=rgba[i+3],expected[4]={a,rgba[i]*a,rgba[i+1]*a,rgba[i+2]*a};auto pixel=direct.data()+y*stride+x*bytes;for(int c=0;c<4;c++)if(bytes==4)check(pixel[c]==pa::toU8(expected[c]*255.f),"ARGB8 exact");else if(bytes==8){std::uint16_t n;std::memcpy(&n,pixel+c*2,2);check(n==std::lround(expected[c]*32768.f),"ARGB16 exact");}else{float n;std::memcpy(&n,pixel+c*4,4);check(n==expected[c],"ARGB float exact");}}
   for(int i=w*bytes;i<stride;i++)check(direct[y*stride+i]==0xCD,"row padding retained");}
 }
 std::cout<<"Streaming parity, 4K/1MiB, preview/PAR/alpha and direct ARGB passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
