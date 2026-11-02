#include "library.hpp"
#define FYLM_SHADERGLASS 1
#include "../plugin/ae/FYLMParams.hpp"
#include "json.hpp"
#include <cmath>
#include <iostream>
#include <set>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 require(fylm::index("sg.preset")==2&&fylm::Params[1].value==1&&fylm::Params[1].choices==39,"selector identity");
 require(fylm::index("sg.virtual_height")==3&&fylm::index("sg.use_slots")==4&&fylm::index("sg.slot_a")==5&&fylm::index("sg.slot_h")==12&&fylm::index("sg.data")==13&&fylm::index("sg.load")==14,"old parameter identities");
 require(fylm::index("sg.defaults")==15&&fylm::index("sg.follow_preset")==16,"new controls must append");
 auto panel=nlohmann::json::parse(fylm::sg::LibraryPanelJson);require(panel.size()==39,"menu count");
 require(panel[0]["menu"]==1&&panel[1]["menu"]==2&&panel[2]["menu"]==3&&panel[2]["id"]=="FYLM.SG.Imported","frozen selection identities");
 require(fylm::sg::recommendedHeight(1,333,true)==333&&fylm::sg::recommendedHeight(3,444,true)==444,"old saved resolution changed");
 require(fylm::sg::recommendedHeight(4,240,true)==720&&fylm::sg::recommendedHeight(4,333,false)==333,"art resolution override");
 retro::RenderRequest r;r.size={160,96};r.timeScale=24;r.input.resize(160*96*4);
 for(int y=0;y<96;y++)for(int x=0;x<160;x++){int i=(y*160+x)*4;r.input[i]=x/159.f;r.input[i+1]=y/95.f;r.input[i+2]=(x/8+y/8)%2?.8f:.2f;r.input[i+3]=1;}
 std::set<std::string> ids;
 for(int menu=1;menu<=fylm::sg::LibraryMenuCount;menu++)if(menu!=3){
  auto p=fylm::sg::builtinPreset(menu);require(p&&!p->passes.empty()&&ids.insert(p->id).second,"missing/duplicate preset identity");
  for(auto& name:p->slots){bool found=false;for(auto& u:p->controls)if(u.name==name)found=true;require(found,"dangling slot mapping");}
  fylm::sg::Settings s;s.virtualHeight=fylm::sg::recommendedHeight(menu,48,true);
  auto a=fylm::sg::render(*p,r,s);require(a.ok,"builtin D3D render");auto b=fylm::sg::render(*p,r,s);require(b.ok&&a.pixels==b.pixels,"builtin repeat");
  double energy=0;for(std::size_t i=0;i<a.pixels.size();i++){require(std::isfinite(a.pixels[i]),"builtin nonfinite");if(i%4!=3)energy+=std::abs(a.pixels[i]);}require(energy>1,"builtin blank");
  s.frameIndex=9;s.timeSeconds=.375f;auto different=fylm::sg::render(*p,r,s);require(different.ok,"alternate frame render");s.frameIndex=0;s.timeSeconds=0;auto repeat=fylm::sg::render(*p,r,s);require(repeat.ok&&repeat.pixels==a.pixels,"builtin out-of-order state");
  if(menu>=4&&menu<=7){s.slots=true;for(int slot=0;slot<16;slot++)if(auto c=fylm::sg::slotControl(*p,slot))s.values[slot]=c->high>c->low?100*(c->value-c->low)/(c->high-c->low):0;auto defaults=fylm::sg::render(*p,r,s);require(defaults.ok,"art default overrides");for(std::size_t i=0;i<a.pixels.size();i++)require(std::abs(defaults.pixels[i]-a.pixels[i])<1e-4,"art slot defaults changed look");}
  if(argc==2){auto path=std::filesystem::path(argv[1])/std::to_string(menu);std::filesystem::create_directories(path.parent_path());fylm::sg::save(*p,path);auto restored=fylm::sg::load(path);require(restored->name==p->name&&restored->slots==p->slots,"recipe persistence");fylm::sg::Settings saved;saved.virtualHeight=fylm::sg::recommendedHeight(menu,48,true);auto image=fylm::sg::render(*restored,r,saved);require(image.ok&&image.pixels==a.pixels,"saved recipe lost defaults");}
 }
 bool rejected=false;try{fylm::sg::builtinPreset(3);}catch(...){rejected=true;}require(rejected,"Imported silently becomes builtin");
 fylm::sg::Settings bypass;bypass.mix=0;require(fylm::sg::render(*fylm::sg::builtinPreset(4),r,bypass).pixels==r.input,"zero strength is not exact bypass");
 require(fylm::sg::builtinPreset(4)->id!=fylm::sg::builtinPreset(2)->id,"recipe/source cache collision");
 std::cout<<"29 embedded presets: stable old selections, source defaults, named slots, D3D replay and save/restore passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
