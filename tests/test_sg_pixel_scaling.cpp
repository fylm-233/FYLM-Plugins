#include "library.hpp"
#include <cmath>
#include <iostream>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 retro::RenderRequest r;r.size={768,512};r.timeScale=24;r.input.resize(768*512*4);
 for(int y=0;y<512;y++)for(int x=0;x<768;x++){int i=(y*768+x)*4;bool line=(x-y)%19==0||((x/5+y/7)%2)==0;r.input[i]=line?.85f:.15f;r.input[i+1]=y/511.f;r.input[i+2]=x/767.f;r.input[i+3]=1;}
 for(int menu=35;menu<=38;menu++){
  auto p=fylm::sg::builtinPreset(menu);fylm::sg::Settings s;s.virtualHeight=fylm::sg::recommendedHeight(menu,0,true);require(s.virtualHeight==240,"pixel recipe missing explicit low-res input");
  auto a=fylm::sg::render(*p,r,s),b=fylm::sg::render(*p,r,s);require(a.ok&&b.ok&&a.pixels==b.pixels,"pixel reconstruction render or repeat");double difference=0;for(std::size_t i=0;i<r.input.size();i++)difference+=std::abs(a.pixels[i]-r.input[i]);difference/=r.input.size();require(difference>.01,"pixel reconstruction is only an identity/no-op");
  s.virtualHeight=360;auto changed=fylm::sg::render(*p,r,s);require(changed.ok&&changed.pixels!=a.pixels,"manual input grid ineffective");
 }
 std::cout<<"4 pixel-grid recipes visibly change diagnostic input and respond to manual grid height\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
