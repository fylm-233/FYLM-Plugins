#include "library.hpp"
#include "timing.hpp"
#include <cmath>
#include <iostream>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 using namespace fylm::sg;
 auto standard=sampleTime(12,24,1,true,1,0,0);require(standard.frame==12&&standard.seconds==.5f&&standard.frameRate==24,"default time mapping changed");
 auto fast=sampleTime(12,24,1,true,2,1,3);require(fast.frame==51&&std::abs(fast.seconds-2.125f)<1e-6,"time rate/offset/phase");
 auto paused=sampleTime(12,24,1,false,2,1,0),pausedLater=sampleTime(24,24,1,false,2,1,0);require(paused.frame==pausedLater.frame&&paused.seconds==pausedLater.seconds,"freeze depends on current frame");
 require(sampleTime(12,24,1,true,-1,0,0).frame==-12,"negative speed");
 for(int frame=0;frame<1000;frame++)require(sampleTime(frame*1001,24000,1001,true,1,0,0).frame==frame,"fractional-frame-rate roundoff");
 retro::RenderRequest r;r.size={192,128};r.timeScale=24;r.input.resize(192*128*4);
 for(int y=0;y<128;y++)for(int x=0;x<192;x++){auto i=(y*192+x)*4;r.input[i]=x/191.f;r.input[i+1]=y/127.f;r.input[i+2]=(x/8+y/8)%2?.8f:.2f;r.input[i+3]=1;}
 for(int menu=31;menu<=34;menu++){
  auto p=builtinPreset(menu);require(p->slots.size()==16,"dynamic preset must expose16 named controls");
  Settings s;s.virtualHeight=64;auto a=render(*p,r,s);require(a.ok,"dynamic shader render");s.frameIndex=12;s.timeSeconds=.5f;auto moving=render(*p,r,s);require(moving.ok&&moving.pixels!=a.pixels,"dynamic shader stays static");s.frameIndex=0;s.timeSeconds=0;auto again=render(*p,r,s);require(again.ok&&again.pixels==a.pixels,"time replay differs");
  s.slots=true;for(int slot=0;slot<16;slot++){auto c=slotControl(*p,slot);require(c,"missing named dynamic control");s.values[slot]=c->high>c->low?100*(c->value-c->low)/(c->high-c->low):0;}
  auto normal=render(*p,r,s);s.values[12]=50;auto extrasOff=render(*p,r,s);require(normal.ok&&extrasOff.ok&&normal.pixels==extrasOff.pixels,"disabled extra slots affect legacy override scope");
  s.extendedSlots=true;auto extrasOn=render(*p,r,s);require(extrasOn.ok&&extrasOn.pixels!=extrasOff.pixels,"I-P extra bank is ineffective");
  s.timeSeconds=-.5f;s.frameIndex=-12;auto reversed=render(*p,r,s);require(reversed.ok,"negative dynamic time rejected");for(float v:reversed.pixels)require(std::isfinite(v),"nonfinite dynamic output");
 }
 std::cout<<"SG time rate/offset/freeze/phase/fractional-fps,4 dynamic presets and16 named slots passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
