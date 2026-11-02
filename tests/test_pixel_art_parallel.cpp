#include "pixel_art/core.hpp"
#include "pixel_art/execution.hpp"
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace pa=pixel_art;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
struct Monitor:pa::IProgress {
 std::thread::id caller=std::this_thread::get_id();int calls=0;bool cancel=false;
 bool cancelled()override{check(std::this_thread::get_id()==caller,"abort callback escaped caller thread");return cancel&&++calls>10;}
 void report(double)override{check(std::this_thread::get_id()==caller,"progress callback escaped caller thread");}
};
int main(){try{
 for(auto entry:std::vector<std::vector<unsigned>>{{1,1,1},{2,2,1},{4,2,1},{4,4,3},{8,4,3},{16,8,7},{32,16,8},{2,16,1},{0,0,1},{32,0,1}})check(pa::executionWorkers({entry[0],entry[1],0})==int(entry[2]),"hardware policy across low/high/unknown capacity");
 check(pa::executionWorkers({32,16,0},2)==2&&pa::executionWorkers({8,4,0},8)==3,"explicit request cannot bypass hardware limit");
 auto capacity=pa::cpuCapacity();std::cout<<"Available logical="<<capacity.logical<<" physical="<<capacity.physical<<" auto="<<pa::executionWorkers(capacity)<<'\n';
 constexpr int w=768,h=512;std::vector<unsigned char> source(w*h*4),reference(source.size()),parallel(source.size());for(int y=0;y<h;y++)for(int x=0;x<w;x++){auto i=(y*w+x)*4;source[i]=static_cast<unsigned char>((x*17+y*3)%256);source[i+1]=static_cast<unsigned char>((x*7+y*19)%256);source[i+2]=static_cast<unsigned char>((x*11+y*5)%256);source[i+3]=static_cast<unsigned char>((x+y)%4?255:64);}
 pa::Params p;p.pc98=true;p.palette=pa::pc98Palette();p.grid.originX=-3;p.grid.originY=2;p.grid.pixelAspect=1.25;
 auto input=[&](const auto& buffer){return pa::ConstImageView{buffer.data(),w,h,std::size_t(w)*4,pa::PixelFormat::RGBA8,-2,4};};auto output=[&](auto& buffer,pa::PixelFormat format=pa::PixelFormat::RGBA8){return pa::ImageView{buffer.data(),w,h,std::size_t(w)*4,format,-2,4};};
 for(double preview:{1.,.5,.25,.2})for(int block:{1,3}){
  p.grid.downsampleX=p.grid.downsampleY=preview;p.grid.block=block;
  for(auto format:{pa::PixelFormat::RGBA8,pa::PixelFormat::ARGB8Premultiplied}){p.workers=1;check(pa::render(input(source),output(reference,format),p).ok,"serial reference");p.workers=8;Monitor progress;check(pa::render(input(source),output(parallel,format),p,&progress).ok&&parallel==reference,"parallel/serial parity with phase, alpha, PAR and native output");}
 }
 p.grid.downsampleX=p.grid.downsampleY=1;p.grid.block=1;p.workers=1;check(pa::render(input(source),output(reference),p).ok,"baseline reset");p.workers=8;
 std::vector<std::future<std::vector<unsigned char>>> frames;for(int n=0;n<4;n++)frames.push_back(std::async(std::launch::async,[&]{auto pixels=parallel;Monitor monitor;check(pa::render(input(source),output(pixels),p,&monitor).ok,"concurrent frame render");return pixels;}));for(auto& frame:frames)check(frame.get()==reference,"shared-helper contention changes result");
 Monitor abort;abort.cancel=true;check(pa::render(input(source),output(parallel),p,&abort).code==pa::ErrorCode::Cancelled,"parallel cancellation");Monitor recovered;check(pa::render(input(source),output(parallel),p,&recovered).ok&&parallel==reference,"render after cancellation");
 p.memoryBudget=1024*1024;check(pa::render(input(source),output(parallel),p).ok&&parallel==reference,"low budget uses serial without multiplying memory");
 p.memoryBudget=16;check(pa::render(input(source),output(parallel),p).code==pa::ErrorCode::OutOfMemory,"tiny budget rejected");p.memoryBudget=256*1024*1024;
 auto alias=source;p.workers=8;check(pa::render(input(alias),output(alias),p).ok&&alias==reference,"overlapping buffers fall back safely");
 p.workers=0;check(pa::render(input(source),output(parallel),p).ok&&parallel==reference,"hardware-auto parity");
#ifdef _WIN32
 {GROUP_AFFINITY affinity{};check(GetThreadGroupAffinity(GetCurrentThread(),&affinity)!=0,"read test thread affinity");auto single=affinity.Mask&(~affinity.Mask+1);auto prior=SetThreadAffinityMask(GetCurrentThread(),single);check(prior!=0,"restrict isolated test thread");struct Restore{DWORD_PTR mask;~Restore(){SetThreadAffinityMask(GetCurrentThread(),mask);}}restore{prior};auto restricted=pa::cpuCapacity();check(restricted.logical==1&&restricted.physical==1&&pa::executionWorkers(restricted)==1,"live one-CPU affinity detected dynamically");check(pa::render(input(source),output(parallel),p).ok&&parallel==reference,"auto remains exact on one available CPU");}
#endif
 p.workers=-1;check(pa::render(input(source),output(parallel),p).code==pa::ErrorCode::InvalidParams,"invalid worker count");
 std::cout<<"Parallel parity, shared limits, callback thread, cancellation, budget and overlap passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
