#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace fylm::sg {
struct TimeSample {int frame=0;float seconds=0,frameRate=24;};
inline TimeSample sampleTime(std::int64_t value,std::uint64_t scale,std::int64_t step,bool animate,double rate,double offset,int phase){
 if(!scale||!std::isfinite(rate)||!std::isfinite(offset)||std::abs(rate)>4||std::abs(offset)>3600)throw std::runtime_error("sg.error.uniform");
 long double fps=step>0?static_cast<long double>(scale)/step:24;
 long double frames=step>0?static_cast<long double>(value)/step:static_cast<long double>(value)/scale*24;
 frames=std::floor((animate?frames*rate:0)+offset*fps)+phase;
 long double seconds=(animate?static_cast<long double>(value)/scale*rate:0)+offset+phase/fps;
 if(frames<std::numeric_limits<int>::min()||frames>std::numeric_limits<int>::max()||!std::isfinite(seconds))throw std::runtime_error("sg.error.uniform");
 return {int(frames),float(seconds),float(fps)};
}
}
