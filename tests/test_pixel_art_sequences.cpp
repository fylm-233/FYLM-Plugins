#include "pixel_art/presets.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace pixel_art;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{constexpr int w=96,h=64;const int order[]={7,0,3,1,6,2,5,4};
 for(auto& preset:artPresets())for(int variant=0;variant<2;variant++)for(int scene=0;scene<4;scene++){
  auto p=artPresetParams(preset,variant,1920);p.grid.block=2;std::set<unsigned> colors;for(auto c:p.palette)colors.insert((c.r<<16)|(c.g<<8)|c.b);
  std::vector<std::vector<unsigned char>> inputs(8),outputs(8);
  for(int frame=0;frame<8;frame++){auto& input=inputs[frame];input.resize(w*h*4);outputs[frame].resize(input.size());for(int y=0;y<h;y++)for(int x=0;x<w;x++){int move=frame<3?frame/2:frame*7;bool light=frame==4||frame==5;int cut=frame>=6?1:0;bool foreground=scene==0?(x-move-45)*(x-move-45)+(y-29)*(y-29)<330:scene==1?y>30+(x+move)%17:scene==2?(x+move)%27<3||y%21<3:(x+move)%23<10&&y%19<12;auto c=p.palette[(foreground?(light?p.palette.size()-1:p.palette.size()/2):cut?1:0)%p.palette.size()];auto i=(y*w+x)*4;input[i]=c.r;input[i+1]=c.g;input[i+2]=c.b;input[i+3]=x==0?0:x==1?64:255;}
   check(render({input.data(),w,h,w*4,PixelFormat::RGBA8},{outputs[frame].data(),w,h,w*4,PixelFormat::RGBA8},p).ok,"scene/motion/light/cut render");for(int i=0;i<w*h;i++)if(outputs[frame][i*4+3])check(colors.count((outputs[frame][i*4]<<16)|(outputs[frame][i*4+1]<<8)|outputs[frame][i*4+2]),"sequence left fixed palette");
  }
  for(int frame:order){std::vector<unsigned char> replay(outputs[frame].size());check(render({inputs[frame].data(),w,h,w*4,PixelFormat::RGBA8},{replay.data(),w,h,w*4,PixelFormat::RGBA8},p).ok&&replay==outputs[frame],"out-of-order frame uses implicit history");}
 }
 auto p=artPresetParams(artPreset("PA10"),0,96);check(!p.cleanSpeckles&&!p.cleanSpurs&&!p.cleanStairs,"arcade preset erases isolated effects by default");p.grid.block=1;std::vector<unsigned char> input(w*h*4,255),out(input.size());auto dark=p.palette.front(),bright=p.palette.back();for(int i=0;i<w*h;i++){input[i*4]=dark.r;input[i*4+1]=dark.g;input[i*4+2]=dark.b;}int marker=(32*w+48)*4;input[marker]=bright.r;input[marker+1]=bright.g;input[marker+2]=bright.b;check(render({input.data(),w,h,w*4,PixelFormat::RGBA8},{out.data(),w,h,w*4,PixelFormat::RGBA8},p).ok&&out[marker]==bright.r&&out[marker+1]==bright.g&&out[marker+2]==bright.b,"arcade high-contrast projectile lost");
 std::cout<<"24 variants, four scene fixtures, slow/fast motion, light/cut, fixed colours, out-of-order replay and arcade projectile passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
