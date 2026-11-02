// GPL-3.0: execution semantics adapted from ShaderGlass ShaderPass.cpp.
#include "preset.hpp"
#include "retro/workspace.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
namespace fylm::sg { namespace {
thread_local std::string backend="not initialized";
thread_local std::uint64_t plannedPeak=0;
thread_local std::uint64_t effectiveBudget=0;
// Failures carry a stable i18n key instead of a raw English sentence: the AE adapter
// shows the key through the shared catalog, so every language gets real text.
struct Failure {const char* key;};
[[noreturn]] void fail(const char* key){throw Failure{key};}
void check(HRESULT hr){if(FAILED(hr))fail("sg.error.device");}
struct Texture {int w=0,h=0;bool mip=false;DXGI_FORMAT format{};ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;ComPtr<ID3D11RenderTargetView> target;};
DXGI_FORMAT outputFormat(const Pass& p){
 const std::map<std::string,DXGI_FORMAT> formats{{"R8G8B8A8_UNORM",DXGI_FORMAT_R8G8B8A8_UNORM},{"R8G8B8A8_SRGB",DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},{"R8_UNORM",DXGI_FORMAT_R8_UNORM},{"R8G8_UNORM",DXGI_FORMAT_R8G8_UNORM},{"R16_SFLOAT",DXGI_FORMAT_R16_FLOAT},{"R16G16_SFLOAT",DXGI_FORMAT_R16G16_FLOAT},{"R16G16B16A16_SFLOAT",DXGI_FORMAT_R16G16B16A16_FLOAT},{"R32_SFLOAT",DXGI_FORMAT_R32_FLOAT},{"R32G32_SFLOAT",DXGI_FORMAT_R32G32_FLOAT},{"R32G32B32A32_SFLOAT",DXGI_FORMAT_R32G32B32A32_FLOAT}};
 if(!p.format.empty()){auto it=formats.find(p.format);if(it==formats.end())fail("sg.error.format_unsupported");return it->second;}
 auto on=[&](const char* key){auto it=p.settings.find(key);return it!=p.settings.end()&&it->second=="true";};
 return on("srgb_framebuffer")?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:on("float_framebuffer")?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;
}
unsigned pixelBytes(DXGI_FORMAT f){switch(f){case DXGI_FORMAT_R8_UNORM:return 1;case DXGI_FORMAT_R8G8_UNORM:case DXGI_FORMAT_R16_FLOAT:return 2;case DXGI_FORMAT_R16G16_FLOAT:case DXGI_FORMAT_R32_FLOAT:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return 4;case DXGI_FORMAT_R32G32_FLOAT:case DXGI_FORMAT_R16G16B16A16_FLOAT:return 8;default:return 16;}}
std::uint64_t textureBytes(int w,int h,DXGI_FORMAT f,bool mip){std::uint64_t bytes=0;do{bytes+=std::uint64_t(w)*h*pixelBytes(f);if(!mip||w==1&&h==1)break;w=std::max(1,w/2);h=std::max(1,h/2);}while(true);return bytes;}
struct Program {ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11InputLayout> layout;ComPtr<ID3D11Buffer> buffers[2];std::vector<unsigned char> scratch[2];UINT bindings[2][2]={{0,1},{0,1}};ComPtr<ID3D11SamplerState> sampler;};
struct Engine {
 std::string backendName;
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;ComPtr<ID3D11Buffer> vertices;
 ComPtr<ID3D11VertexShader> copyVs;ComPtr<ID3D11PixelShader> copyPs;ComPtr<ID3D11SamplerState> linear;
 std::map<std::string,std::vector<Program>> programs;std::map<std::string,Texture> textures;ComPtr<ID3D11Texture2D> staging;int stageW=0,stageH=0;std::string activePreset;
 std::uint64_t budget(std::uint64_t requested){MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(GlobalMemoryStatusEx(&memory))requested=std::min<std::uint64_t>(requested,memory.ullAvailPhys/3);ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<IDXGIAdapter3> modern;DXGI_QUERY_VIDEO_MEMORY_INFO info{};if(SUCCEEDED(device.As(&dxgi))&&SUCCEEDED(dxgi->GetAdapter(&adapter))&&SUCCEEDED(adapter.As(&modern))&&SUCCEEDED(modern->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info))&&info.Budget){auto available=info.Budget>info.CurrentUsage?info.Budget-info.CurrentUsage:0;requested=std::min<std::uint64_t>(requested,available*4/5);}return requested;}
 void releaseFrame()noexcept{context->ClearState();textures.clear();staging.Reset();stageW=stageH=0;context->Flush();}
 Engine(){D3D_FEATURE_LEVEL level;auto h=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context);if(FAILED(h)){check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));backend="D3D11 WARP";}else backend="D3D11 hardware";
  ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC description{};if(SUCCEEDED(device.As(&dxgi))&&SUCCEEDED(dxgi->GetAdapter(&adapter))&&SUCCEEDED(adapter->GetDesc(&description))){char name[512]{};WideCharToMultiByte(CP_UTF8,0,description.Description,-1,name,sizeof(name),nullptr,nullptr);backend+=" / "+std::string(name);}
  float v[]={0,0,0,1,0,1, 0,1,0,1,0,0, 1,0,0,1,1,1, 1,1,0,1,1,0};D3D11_BUFFER_DESC b{};b.ByteWidth=sizeof(v);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA data{};data.pSysMem=v;check(device->CreateBuffer(&b,&data,&vertices));
  const char* code="Texture2D t:register(t0);SamplerState s:register(s0);struct V{float4 p:SV_POSITION;float2 uv:TEXCOORD0;};V vs(uint i:SV_VertexID){V v;v.uv=float2((i<<1)&2,i&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}float4 ps(V v):SV_TARGET{return t.Sample(s,v.uv);}";
  for(int i=0;i<2;i++){ComPtr<ID3DBlob> blob,error;check(D3DCompile(code,std::strlen(code),nullptr,nullptr,nullptr,i?"ps":"vs",i?"ps_5_0":"vs_5_0",0,0,&blob,&error));if(i)check(device->CreatePixelShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&copyPs));else check(device->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&copyVs));}
  D3D11_SAMPLER_DESC s{};s.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;s.MaxLOD=D3D11_FLOAT32_MAX;check(device->CreateSamplerState(&s,&linear));backendName=backend;
 }
 Texture& texture(const std::string& name,int w,int h,DXGI_FORMAT format,bool mip=false){
  if(w<1||h<1||w>16384||h>16384)fail("sg.error.size");
  auto& t=textures[name];if(t.w==w&&t.h==h&&t.format==format&&t.mip==mip)return t;
  UINT support=0;check(device->CheckFormatSupport(format,&support));UINT required=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_SAMPLE|D3D11_FORMAT_SUPPORT_RENDER_TARGET;if(mip)required|=D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;if((support&required)!=required)fail("sg.error.format_unsupported");
  t={};t.w=w;t.h=h;t.format=format;t.mip=mip;D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=mip?0:1;d.ArraySize=1;d.MiscFlags=mip?D3D11_RESOURCE_MISC_GENERATE_MIPS:0;d.Format=format;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
  check(device->CreateTexture2D(&d,nullptr,&t.texture));check(device->CreateShaderResourceView(t.texture.Get(),nullptr,&t.view));check(device->CreateRenderTargetView(t.texture.Get(),nullptr,&t.target));return t;
 }
 void viewport(Texture& target){auto rt=target.target.Get();context->OMSetRenderTargets(1,&rt,nullptr);D3D11_VIEWPORT v{0,0,float(target.w),float(target.h),0,1};context->RSSetViewports(1,&v);}
 void detach(){ID3D11ShaderResourceView* nulls[16]{};context->PSSetShaderResources(0,16,nulls);context->VSSetShaderResources(0,16,nulls);ID3D11RenderTargetView* rt=nullptr;context->OMSetRenderTargets(1,&rt,nullptr);}
 void copy(Texture& input,Texture& output){viewport(output);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(copyVs.Get(),nullptr,0);context->PSSetShader(copyPs.Get(),nullptr,0);auto view=input.view.Get();auto sampler=linear.Get();context->PSSetShaderResources(0,1,&view);context->PSSetSamplers(0,1,&sampler);context->Draw(3,0);detach();}
 std::vector<Program>& prepare(const Preset& preset){
  auto it=programs.find(preset.id);if(it!=programs.end())return it->second;
  std::vector<Program> result;
  for(auto& pass:preset.passes){Program p;check(device->CreateVertexShader(pass.vertex.data(),pass.vertex.size(),nullptr,&p.vs));check(device->CreatePixelShader(pass.fragment.data(),pass.fragment.size(),nullptr,&p.ps));
   D3D11_INPUT_ELEMENT_DESC layout[]={{"TEXCOORD",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0}};check(device->CreateInputLayout(layout,2,pass.vertex.data(),pass.vertex.size(),&p.layout));
   for(int i=0;i<2;++i){int bytes=0;for(auto& u:pass.uniforms)if(u.buffer==(i? -1:0))bytes=std::max(bytes,u.offset+u.size);if(bytes){D3D11_BUFFER_DESC d{};d.ByteWidth=(bytes+15)&~15;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device->CreateBuffer(&d,nullptr,&p.buffers[i]));p.scratch[i].resize(d.ByteWidth);}}
   // Older packages could auto-assign a push-only stage to b0. Read its actual
   // DXBC binding rather than silently binding every push block to b1.
   if(!p.buffers[0]&&p.buffers[1])for(int stage=0;stage<2;stage++){auto& bytes=stage?pass.fragment:pass.vertex;ComPtr<ID3D11ShaderReflection> reflection;check(D3DReflect(bytes.data(),bytes.size(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(reflection.GetAddressOf())));D3D11_SHADER_DESC desc{};check(reflection->GetDesc(&desc));int count=0;for(UINT i=0;i<desc.BoundResources;i++){D3D11_SHADER_INPUT_BIND_DESC binding{};check(reflection->GetResourceBindingDesc(i,&binding));if(binding.Type!=D3D_SIT_CBUFFER)continue;if(++count>1)fail("sg.error.uniform");p.bindings[stage][1]=binding.BindPoint;D3D11_SHADER_BUFFER_DESC buffer{};check(reflection->GetConstantBufferByName(binding.Name)->GetDesc(&buffer));D3D11_BUFFER_DESC allocated{};p.buffers[1]->GetDesc(&allocated);if(buffer.Size>allocated.ByteWidth)fail("sg.error.uniform");}}
   auto setting=[&](const char* key){auto it=pass.settings.find(key);return it==pass.settings.end()?std::string():it->second;};
   D3D11_SAMPLER_DESC s{};s.Filter=setting("filter_linear")=="true"?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;
   if(setting("wrap_mode")=="clamp_to_edge")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
   else if(setting("wrap_mode")=="repeat")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
   else if(setting("wrap_mode")=="mirrored_repeat")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_MIRROR;
   s.MaxLOD=D3D11_FLOAT32_MAX;check(device->CreateSamplerState(&s,&p.sampler));result.push_back(std::move(p));
  }
  if(programs.size()>=8)programs.clear();return programs.emplace(preset.id,std::move(result)).first->second;
 }
 retro::RenderOutput run(const Preset& preset,const retro::RenderRequest& r,const Settings& settings){
  if(r.size.width<=0||r.size.height<=0||r.input.size()!=std::size_t(r.size.width)*r.size.height*4)fail("sg.error.input");
  int vh=settings.virtualHeight>0?std::min(settings.virtualHeight,r.size.height):r.size.height;int vw=std::max(1,int(std::round(double(r.size.width)*vh/r.size.height)));
  // Size every pass before touching the device: a preset that cannot fit its own budget is
  // rejected without allocating and without leaving half-built textures behind.
  struct Plan{int w,h;DXGI_FORMAT format;};
  std::vector<Plan> plans;plans.reserve(preset.passes.size());
  effectiveBudget=budget(r.memoryBudget);
  std::vector<std::size_t> lastUse(preset.passes.size());std::map<std::string,std::size_t> outputs;
  for(std::size_t i=0;i<preset.passes.size();i++){lastUse[i]=i+1;outputs["PassOutput"+std::to_string(i)]=i;auto alias=preset.passes[i].settings.find("alias");if(alias!=preset.passes[i].settings.end()&&!alias->second.empty())outputs[alias->second]=i;}
  for(std::size_t i=0;i<preset.passes.size();i++){auto use=[&](const std::string& name){auto it=outputs.find(name);if(it!=outputs.end())lastUse[it->second]=std::max(lastUse[it->second],i);};for(auto& sampler:preset.passes[i].samplers)use(sampler.name);for(auto& u:preset.passes[i].uniforms)if(u.description.empty()){if(u.name.starts_with("PassOutputSize"))use("PassOutput"+u.name.substr(14));else if(u.name.ends_with("Size"))use(u.name.substr(0,u.name.size()-4));}}
  bool chainMip=std::any_of(preset.passes.begin(),preset.passes.end(),[](const auto& p){auto it=p.settings.find("mipmap_input");return it!=p.settings.end()&&it->second=="true";});
  // Caller input/output plus upload/final/staging GPU textures.
  std::uint64_t peak=std::uint64_t(r.size.width)*r.size.height*16*5+textureBytes(vw,vh,DXGI_FORMAT_R32G32B32A32_FLOAT,chainMip);
  for(auto& asset:preset.resources){auto on=[&](const char* key){auto it=asset.settings.find(key);return it!=asset.settings.end()&&it->second=="true";};peak+=asset.encoded.size()+asset.rgba.size()+textureBytes(asset.width,asset.height,on("srgb")?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_R8G8B8A8_UNORM,on("mipmap"));}
  plannedPeak=peak;if(peak>effectiveBudget)fail("sg.error.budget");
  const auto base=peak;std::uint64_t live=0;int planW=vw,planH=vh;
  for(std::size_t i=0;i<preset.passes.size();++i){
   const auto& p=preset.passes[i];
   auto get=[&](const std::string& key,const std::string& fallback=""){auto it=p.settings.find(key);return it==p.settings.end()?fallback:it->second;};
   auto number=[&](const std::string& text){try{return std::stof(text);}catch(const std::exception&){fail("sg.error.scale");}return 0.f;};
   auto dimension=[&](const char* axis,int source,int viewport){std::string suffix=std::string("_")+axis;auto type=get("scale_type"+suffix,get("scale_type",i+1==preset.passes.size()?"viewport":"source"));float scale=number(get("scale"+suffix,get("scale","1")));if(!std::isfinite(scale)||scale<=0||scale>16384)fail("sg.error.scale");double value=type=="absolute"?scale:type=="viewport"?viewport*double(scale):type=="source"?source*double(scale):-1;if(value<1||value>16384)fail("sg.error.scale");return int(std::round(value));};
   int w=dimension("x",planW,r.size.width),h=dimension("y",planH,r.size.height);
   auto format=outputFormat(p);
   live+=textureBytes(w,h,format,chainMip);peak=std::max(peak,base+live);
   plannedPeak=peak;if(peak>effectiveBudget)fail("sg.error.budget");
   plans.push_back({w,h,format});planW=w;planH=h;
   for(std::size_t j=0;j<i;j++)if(lastUse[j]==i)live-=textureBytes(plans[j].w,plans[j].h,plans[j].format,chainMip);
  }
  context->ClearState();if(activePreset!=preset.id){textures.clear();staging.Reset();stageW=stageH=0;activePreset=preset.id;}auto& gpu=prepare(preset);auto& original=texture("original",r.size.width,r.size.height,DXGI_FORMAT_R32G32B32A32_FLOAT);context->UpdateSubresource(original.texture.Get(),0,nullptr,r.input.data(),original.w*16,0);
  auto& virtualSource=texture("virtual",vw,vh,DXGI_FORMAT_R32G32B32A32_FLOAT,chainMip);copy(original,virtualSource);if(chainMip)context->GenerateMips(virtualSource.view.Get());
  Texture* current=&virtualSource;std::map<std::string,Texture*> sources{{"Original",&virtualSource},{"OriginalHistory0",&virtualSource}};
  std::map<std::string,ComPtr<ID3D11SamplerState>> assetSamplers;
  for(auto& asset:preset.resources){auto value=[&](const char* key){auto it=asset.settings.find(key);return it==asset.settings.end()?std::string():it->second;};auto& t=texture("asset:"+asset.name,asset.width,asset.height,value("srgb")=="true"?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_R8G8B8A8_UNORM,value("mipmap")=="true");context->UpdateSubresource(t.texture.Get(),0,nullptr,asset.rgba.data(),asset.width*4,0);if(t.mip)context->GenerateMips(t.view.Get());sources[asset.name]=&t;
   D3D11_SAMPLER_DESC s{};s.Filter=value("linear")=="true"?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;if(value("wrap_mode")=="clamp_to_edge")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;else if(value("wrap_mode")=="repeat")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;else if(value("wrap_mode")=="mirrored_repeat")s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_MIRROR;s.MaxLOD=t.mip?D3D11_FLOAT32_MAX:0;check(device->CreateSamplerState(&s,&assetSamplers[asset.name]));
  }
  for(std::size_t i=0;i<preset.passes.size();++i){auto& p=preset.passes[i];auto& program=gpu[i];
   auto get=[&](const std::string& key,const std::string& fallback=""){auto it=p.settings.find(key);return it==p.settings.end()?fallback:it->second;};
   int w=plans[i].w,h=plans[i].h;auto format=plans[i].format;
   if(r.cancelled&&r.cancelled())fail("retro.error.cancelled");
   auto& destination=texture("pass"+std::to_string(i),w,h,format,chainMip);sources["Source"]=current;
   for(int buffer=0;buffer<2;++buffer)if(program.buffers[buffer]){
    auto& bytes=program.scratch[buffer];std::fill(bytes.begin(),bytes.end(),0);
    for(auto& u:p.uniforms)if(u.buffer==(buffer?-1:0)){
     if(u.offset<0||u.size<0||std::size_t(u.offset+u.size)>bytes.size())fail("sg.error.uniform");auto ptr=bytes.data()+u.offset;
     if(u.name=="MVP"){float m[16]={2,0,0,0,0,2,0,0,0,0,0,0,-1,-1,0,1};if(u.size==64)std::memcpy(ptr,m,64);}
     else if(u.name=="FrameCount"){int frame=settings.frameIndex;auto mod=get("frame_count_mod");if(!mod.empty()){int n=0;try{n=std::stoi(mod);}catch(const std::exception&){fail("sg.error.scale");}if(n>0)frame=((frame%n)+n)%n;}if(u.size==4)std::memcpy(ptr,&frame,4);}
     else if(u.name=="FrameTime"){if(u.size!=4)fail("sg.error.uniform");std::memcpy(ptr,&settings.timeSeconds,4);}
     else if(u.name=="FrameRate"){if(u.size!=4)fail("sg.error.uniform");std::memcpy(ptr,&settings.frameRate,4);}
     else if(u.name=="FrameDirection"){if(u.size!=4)fail("sg.error.uniform");std::memcpy(ptr,&settings.frameDirection,4);}
     else if(u.name=="Rotation"){int rotation=0;if(u.size!=4)fail("sg.error.uniform");std::memcpy(ptr,&rotation,4);}
     else if(u.name=="OriginalAspectRatio"){float aspect=float(vw)*settings.pixelAspect/vh;if(u.size!=4)fail("sg.error.uniform");std::memcpy(ptr,&aspect,4);}
     else if(u.name=="OutputSize"||u.name=="FinalViewportSize"){float size[4]={float(u.name=="OutputSize"?w:r.size.width),float(u.name=="OutputSize"?h:r.size.height),1.f/(u.name=="OutputSize"?w:r.size.width),1.f/(u.name=="OutputSize"?h:r.size.height)};std::memcpy(ptr,size,std::min(u.size,16));}
     else if(u.description.empty()&&(u.name.ends_with("Size")||u.name.starts_with("PassOutputSize"))){auto key=u.name.starts_with("PassOutputSize")?"PassOutput"+u.name.substr(14):u.name.substr(0,u.name.size()-4);auto it=sources.find(key);if(it==sources.end())fail("sg.error.uniform");auto t=it->second;float size[4]={float(t->w),float(t->h),1.f/t->w,1.f/t->h};std::memcpy(ptr,size,std::min(u.size,16));}
     else {float value=u.value;if(settings.slots)for(std::size_t slot=0;slot<std::size_t(settings.extendedSlots?16:8);++slot){auto control=slotControl(preset,slot);if(control&&control->name==u.name)value=u.low+(u.high-u.low)*std::clamp(settings.values[slot],0.f,100.f)/100.f;}if(auto named=settings.namedValues.find(u.name);named!=settings.namedValues.end()){if(!std::isfinite(named->second)||named->second<u.low||named->second>u.high)fail("sg.error.uniform");value=named->second;}if(u.size!=4||u.description.empty())fail("sg.error.uniform");std::memcpy(ptr,&value,4);}
    }context->UpdateSubresource(program.buffers[buffer].Get(),0,nullptr,bytes.data(),0,0);auto cb=program.buffers[buffer].Get();context->VSSetConstantBuffers(program.bindings[0][buffer],1,&cb);context->PSSetConstantBuffers(program.bindings[1][buffer],1,&cb);
   }
   viewport(destination);UINT stride=24,offset=0;auto vb=vertices.Get();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetInputLayout(program.layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);context->VSSetShader(program.vs.Get(),nullptr,0);context->PSSetShader(program.ps.Get(),nullptr,0);
   for(auto& sampler:p.samplers){auto it=sources.find(sampler.name);if(it==sources.end())fail("sg.error.texture");auto srv=it->second->view.Get();auto asset=assetSamplers.find(sampler.name);auto ss=asset==assetSamplers.end()?program.sampler.Get():asset->second.Get();context->PSSetShaderResources(sampler.binding,1,&srv);context->PSSetSamplers(sampler.binding,1,&ss);context->VSSetShaderResources(sampler.binding,1,&srv);context->VSSetSamplers(sampler.binding,1,&ss);}
   context->Draw(4,0);detach();if(chainMip)context->GenerateMips(destination.view.Get());current=&destination;sources["PassOutput"+std::to_string(i)]=current;auto alias=get("alias");if(!alias.empty())sources[alias]=current;
   for(std::size_t j=0;j<i;j++)if(lastUse[j]==i)textures.erase("pass"+std::to_string(j));
  }
  auto& final=texture("final",r.size.width,r.size.height,DXGI_FORMAT_R32G32B32A32_FLOAT);copy(*current,final);
  if(stageW!=final.w||stageH!=final.h){staging.Reset();D3D11_TEXTURE2D_DESC d{};final.texture->GetDesc(&d);d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check(device->CreateTexture2D(&d,nullptr,&staging));stageW=final.w;stageH=final.h;}
  context->CopyResource(staging.Get(),final.texture.Get());retro::RenderOutput result;result.pixels.resize(r.input.size());D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
  for(int y=0;y<final.h;++y)std::memcpy(result.pixels.data()+std::size_t(y)*final.w*4,static_cast<unsigned char*>(mapped.pData)+std::size_t(y)*mapped.RowPitch,final.w*16);context->Unmap(staging.Get(),0);result.ok=true;return result;
 }
};
}
std::string rendererBackend(){return backend;}
std::uint64_t rendererPeakBytes(){return plannedPeak;}
std::uint64_t rendererBudgetBytes(){return effectiveBudget;}
retro::RenderOutput render(const Preset& preset,const retro::RenderRequest& r,const Settings& settings){try{
 requireRuntimeSupport(preset);
 if(!std::isfinite(settings.pixelAspect)||settings.pixelAspect<=0||!std::isfinite(settings.frameRate)||settings.frameRate<=0||(settings.frameDirection!=1&&settings.frameDirection!=-1))return {false,"sg.error.uniform",{}};
 for(auto& [name,value]:settings.namedValues){auto found=std::find_if(preset.controls.begin(),preset.controls.end(),[&](const auto& u){return u.name==name;});if(found==preset.controls.end()||!std::isfinite(value)||value<found->low||value>found->high)return {false,"sg.error.uniform",{}};}
 if(r.cancelled&&r.cancelled())return {false,"retro.error.cancelled",{}};
 if(!std::isfinite(settings.mix)||settings.mix<0||settings.mix>1)return {false,"sg.error.uniform",{}};
 const float mix=settings.mix*preset.mix;if(!std::isfinite(mix)||mix<0||mix>1)return {false,"sg.error.uniform",{}};
 if(mix==0&&r.size.width>0&&r.size.height>0&&r.input.size()==std::size_t(r.size.width)*r.size.height*4)return {true,"",r.input};
 retro::RenderOutput out;
 {retro::WorkspaceLease lease;if(!lease.acquire(r.cancelled))return {false,"retro.error.cancelled",{}};
  // WorkspaceLease serializes all FYLM staging users. One shared SG device
  // bounds device/program caches independently of AE's worker/CPU thread count.
  static std::unique_ptr<Engine> engine;if(!engine)engine=std::make_unique<Engine>();backend=engine->backendName;
  try{{struct Trim{Engine& e;~Trim(){e.releaseFrame();}} trim{*engine};out=engine->run(preset,r,settings);}}
  catch(const Failure&){if(FAILED(engine->device->GetDeviceRemovedReason()))engine.reset();throw;}
  catch(...){engine.reset();throw;}}
 if(r.cancelled&&r.cancelled())return {false,"retro.error.cancelled",{}};
 // Display strength blends in the host's encoded SDR domain. Clamp the filtered
 // branch before blending so ringing highlights do not overpower the original.
 if(out.ok&&mix<1)for(std::size_t i=0;i<out.pixels.size();i++)out.pixels[i]=i%4==3?r.input[i]:r.input[i]*(1-mix)+std::clamp(out.pixels[i],0.f,1.f)*mix;
 return out;
}catch(const Failure& f){return {false,f.key,{}};}
 catch(const std::bad_alloc&){return {false,"sg.error.memory",{}};}
 catch(const std::exception& error){auto message=std::string(error.what());return {false,message.starts_with("sg.error.")?message:"sg.error.device",{}};}}
}
