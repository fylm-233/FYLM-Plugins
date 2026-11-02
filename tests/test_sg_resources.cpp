#include "preset.hpp"
#include <cmath>
#include <iostream>
#include <future>
#include <d3dcompiler.h>
#include <cstring>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 if(argc!=2)return 2;auto original=fylm::sg::load(argv[1]);require(original->resources.size()==1&&original->resources[0].width==2,"fixture resource decode");
 retro::RenderRequest r;r.size={96,64};r.input.resize(96*64*4);for(std::size_t i=0;i<r.input.size();i+=4){r.input[i]=.2f;r.input[i+1]=.4f;r.input[i+2]=.6f;r.input[i+3]=1;}
 fylm::sg::Settings s;s.virtualHeight=32;auto baseline=fylm::sg::render(*original,r,s);require(baseline.ok,"resource/mipmap render failed");require(baseline.pixels!=r.input,"resource ignored");
 auto bytes=fylm::sg::encode(*original);auto restored=fylm::sg::loadBytes(bytes);auto replay=fylm::sg::render(*restored,r,s);require(replay.ok&&replay.pixels==baseline.pixels,"embedded resource round trip");
 {auto legacy=*original;legacy.id+="-push-b0";legacy.resources.clear();auto& pass=legacy.passes[0];pass.uniforms={{"Gain","Gain",-1,0,4,0,1,.25f}};pass.samplers.clear();legacy.controls=pass.uniforms;legacy.slots={"Gain"};const char* code="cbuffer Push:register(b0){float Gain;}float4 main():SV_TARGET{return float4(Gain,Gain,Gain,1);}";ID3DBlob* shader=nullptr,*error=nullptr;auto hr=D3DCompile(code,std::strlen(code),nullptr,nullptr,nullptr,"main","ps_5_0",0,0,&shader,&error);if(error)error->Release();require(SUCCEEDED(hr),"legacy shader compile");auto start=static_cast<const unsigned char*>(shader->GetBufferPointer());pass.fragment.assign(start,start+shader->GetBufferSize());shader->Release();auto a=fylm::sg::render(legacy,r,s);s.namedValues={{"Gain",.75f}};auto b=fylm::sg::render(legacy,r,s);require(a.ok&&b.ok&&std::abs(a.pixels[0]-.25f)<.005f&&std::abs(b.pixels[0]-.75f)<.005f,"legacy push b0 binding ignored parameters");s.namedValues.clear();}
 s.namedValues={{"ResourceLOD",1}};auto mip=fylm::sg::render(*original,r,s);if(mip.ok&&mip.pixels==baseline.pixels){std::cerr<<"mip debug:";for(int i=0;i<8;i++)std::cerr<<' '<<baseline.pixels[i];std::cerr<<" resource:";for(auto value:original->resources[0].rgba)std::cerr<<' '<<int(value);std::cerr<<'\n';}require(mip.ok&&mip.pixels!=baseline.pixels,"generated mip not sampled");for(std::size_t i=0;i<mip.pixels.size();i++)if(i%4!=3)require(std::abs(mip.pixels[i]-(.375f+.25f*r.input[i]))<.005f,"lookup mip average incorrect");s.namedValues.clear();
 auto clamp=*original;clamp.id+="-clamp";clamp.resources[0].settings["wrap_mode"]="clamp_to_edge";auto wrapped=fylm::sg::render(clamp,r,s);require(wrapped.ok&&wrapped.pixels!=baseline.pixels,"resource wrap ignored");
 auto linear=*original;linear.id+="-linear";linear.resources[0].settings["linear"]="true";auto filtered=fylm::sg::render(linear,r,s);require(filtered.ok&&filtered.pixels!=baseline.pixels,"resource filter ignored");
 std::vector<std::future<bool>> workers;for(int i=0;i<4;i++)workers.push_back(std::async(std::launch::async,[&]{auto out=fylm::sg::render(*original,r,s);return out.ok&&out.pixels==baseline.pixels&&fylm::sg::rendererBackend().find("not initialized")==std::string::npos;}));for(auto& worker:workers)require(worker.get(),"shared device worker mismatch");
 auto changed=*original;std::fill(changed.resources[0].rgba.begin(),changed.resources[0].rgba.end(),0);changed.id+="-changed";auto asset=fylm::sg::render(changed,r,s);require(asset.ok&&asset.pixels!=baseline.pixels,"resource binding has no effect");
 for(auto format:{"R8_UNORM","R8G8_UNORM","R16_SFLOAT","R16G16_SFLOAT","R16G16B16A16_SFLOAT","R32_SFLOAT","R32G32_SFLOAT","R32G32B32A32_SFLOAT","R8G8B8A8_UNORM","R8G8B8A8_SRGB"}){auto p=*original;p.passes[0].format=format;p.id+=format;auto out=fylm::sg::render(p,r,s);require(out.ok,"explicit output format failed");for(float v:out.pixels)require(std::isfinite(v),"format produced nonfinite output");}
 auto lowBudget=r;lowBudget.memoryBudget=1024;require(!fylm::sg::render(*original,lowBudget,s).ok,"resource peak budget ignored");
 auto chain=*original;chain.id+="-live";chain.passes.assign(20,original->passes[0]);auto bounded=r;bounded.memoryBudget=600*1024;require(fylm::sg::render(chain,bounded,s).ok,"dead intermediate passes retained beyond budget");
 int abortCalls=0;auto cancelled=r;cancelled.cancelled=[&]{return ++abortCalls>=3;};require(fylm::sg::render(*original,cancelled,s).error=="retro.error.cancelled","pass cancellation ignored");require(fylm::sg::render(*original,r,s).pixels==baseline.pixels,"render did not recover after cancellation");
 auto missing=*original;missing.resources.clear();require(!fylm::sg::render(missing,r,s).ok,"missing LUT silently substituted");
 auto temporal=*original;temporal.passes[0].samplers[1].name="OriginalHistory1";auto rejected=fylm::sg::render(temporal,r,s);require(!rejected.ok&&rejected.error=="sg.error.temporal","history silently substituted");
 std::cout<<"Embedded resources, mipmaps, ten formats, replay and rejection passed: "<<fylm::sg::rendererBackend()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
