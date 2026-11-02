#include "pixel_art/core.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pixel_art {namespace {
std::uint8_t quantize(double value,int maximum,double offset=0)noexcept{
 value=std::isfinite(value)?std::clamp(value,0.,1.):0.;
 auto code=std::clamp(std::floor(value*maximum+.5+offset),0.,double(maximum));
 return toU8(code*255./maximum);
}
}
Rgb8 pc98DirectColor(double r,double g,double b,PC98ColorSpec spec,double offset)noexcept{
 if(!std::isfinite(offset))offset=0;offset=std::clamp(offset,-.5,.5);
 if(spec==PC98ColorSpec::RGB565)return {quantize(r,31,offset),quantize(g,63,offset),quantize(b,31,offset)};
 return {quantize(r,255),quantize(g,255),quantize(b,255)};
}
std::vector<Rgb8> pc98ConformPalette(const std::vector<Rgb8>& colors,PC98ColorSpec spec,int bits){
 if(bits!=6&&bits!=8)throw std::invalid_argument("PC98 DAC bits");
 if(int(spec)<0||int(spec)>5||colors.size()>256)throw std::invalid_argument("PC98 palette specification");
 if(spec==PC98ColorSpec::Digital8)return pc98HardwarePalette(spec,8,bits);
 if(spec==PC98ColorSpec::RGB565||spec==PC98ColorSpec::RGB888)return {};
 auto result=colors;int maximum=spec==PC98ColorSpec::Analog16?15:spec==PC98ColorSpec::Indexed256&&bits==6?63:255;
 if(spec==PC98ColorSpec::Analog16&&result.size()>16)result.resize(16);
 if(spec==PC98ColorSpec::Indexed256&&result.size()>256)result.resize(256);
 if(maximum<255)for(auto& color:result)color={quantize(color.r/255.,maximum),quantize(color.g/255.,maximum),quantize(color.b/255.,maximum)};
 return result;
}
std::vector<Rgb8> pc98HardwarePalette(PC98ColorSpec spec,int count,int bits){
 if(bits!=6&&bits!=8)throw std::invalid_argument("PC98 DAC bits");
 if(spec==PC98ColorSpec::Digital8){std::vector<Rgb8> colors;for(int i=0;i<8;i++)colors.push_back({std::uint8_t(i&4?255:0),std::uint8_t(i&2?255:0),std::uint8_t(i&1?255:0)});return colors;}
 if(spec==PC98ColorSpec::Analog16)return pc98ConformPalette(pc98Palette(),spec,bits);
 if(spec==PC98ColorSpec::Indexed256){if(count<2||count>256)throw std::invalid_argument("PC98 indexed count");std::vector<Rgb8> colors;colors.reserve(count);for(int i=0;i<count;i++){int code=int(std::lround(i*255./(count-1)));colors.push_back({toU8(((code>>5)&7)*255./7),toU8(((code>>2)&7)*255./7),toU8((code&3)*255./3)});}return pc98ConformPalette(colors,spec,bits);}
 if(spec==PC98ColorSpec::Art)return pc98Palette();
 if(spec==PC98ColorSpec::RGB565||spec==PC98ColorSpec::RGB888)return {}; // direct colour is never an enumerated palette
 throw std::invalid_argument("PC98 colour specification");
}
}
