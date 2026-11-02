#define FYLM_CRT_ONLY 1
#include "../plugin/ae/FYLMParams.hpp"
#include "fylm/presets.hpp"
#include "fylm/ui_values.hpp"
#include <cmath>
#include <iostream>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 require(fylm::index("retro.scan_sharpness")==38&&fylm::index("retro.signal_model")==39&&fylm::index("retro.style")==44&&fylm::index("retro.apply_style")==45,"frozen indices / append-only");
 auto frozen=fylm::crt::vhsPresets();require(frozen.size()==8,"recipe count");
 retro::RenderRequest r;r.size={192,128};r.timeScale=24;r.input.resize(192*128*4);
 for(int y=0;y<128;y++)for(int x=0;x<192;x++){auto i=(y*192+x)*4;r.input[i]=x/191.f;r.input[i+1]=y/127.f;r.input[i+2]=(x/4+y/4)%2?.8f:.2f;r.input[i+3]=1;}
 for(auto& recipe:frozen){
  for(auto& [key,value]:recipe.settings){int i=fylm::index(key.c_str());require(i>0,"unknown recipe control");const auto& spec=fylm::Params[i-1];require(value>=spec.low&&value<=spec.high,"recipe range");require(key!="retro.mode"&&key!="retro.vhs"&&key!="retro.crt"&&key!="retro.seed"&&key!="retro.animate"&&key!="retro.curvature"&&key!="retro.scan_opacity"&&key!="retro.mask_intensity","recipe crosses scope");}
  auto value=[&](const char* key)->double{if(auto it=recipe.settings.find(key);it!=recipe.settings.end())return it->second;return fylm::Params[fylm::index(key)-1].value;};auto signal=fylm::crt::signalFromUi(value);r.controls=fylm::crt::controlsFromUi(value);r.controls.crt.crtEnabled=0;r.timeValue=0;
  auto first=fylm::crt::render(r,signal);require(first.ok,"YIQ preset render");for(float v:first.pixels)require(std::isfinite(v),"YIQ nonfinite");r.timeValue=12;auto moving=fylm::crt::render(r,signal);r.timeValue=0;auto again=fylm::crt::render(r,signal);require(moving.ok&&moving.pixels!=first.pixels&&again.ok&&first.pixels==again.pixels,"YIQ temporal determinism");
  r.controls.animateTime=false;auto paused=fylm::crt::render(r,signal);r.timeValue=12;auto pausedAgain=fylm::crt::render(r,signal);require(paused.ok&&pausedAgain.ok&&paused.pixels==pausedAgain.pixels,"freeze ignored");r.controls.animateTime=true;r.timeValue=0;
  signal.seed=37;auto seeded=fylm::crt::render(r,signal);require(seeded.ok&&seeded.pixels!=first.pixels,"seed ineffective");
  r.controls.crt.crtEnabled=1;r.controls.crt.vhsEnabled=0;auto crt=fylm::crt::render(r,{});signal.chromaNoise=1;signal.headSwitch=1;auto noVhs=fylm::crt::render(r,signal);require(crt.ok&&noVhs.ok&&crt.pixels==noVhs.pixels,"advanced VHS leaks into CRT-only");
  r.controls.crt.vhsEnabled=1;signal.strength=0;auto zero=fylm::crt::render(r,signal);require(zero.ok&&zero.pixels==crt.pixels,"zero VHS strength changes CRT stage");
 }
 auto invalid=fylm::crt::SignalSettings{};invalid.strength=2;require(!fylm::crt::render(r,invalid).ok,"invalid strength accepted");
 std::cout<<"8 VHS recipes: scope, YIQ, dynamic/frozen/seeded/reordered frames and CRT independence passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
