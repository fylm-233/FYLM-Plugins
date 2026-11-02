#include "fylm/backend.hpp"
#include "legacy_shader.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstring>
#include <cmath>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
namespace fylm::crt {namespace {
void checked(HRESULT h){if(FAILED(h))throw std::runtime_error("D3D11 operation failed");}
// Built-in spatial CRT/VHS bridge preset; not a crt-geom or arbitrary slang implementation.
const char* shader=R"(
Texture2D source:register(t0);SamplerState sample0:register(s0);
cbuffer Frame:register(b0){float4 geometry;float4 motion;float4 signal;float4 tracking;float4 streak;float4 dropout;float4 screen;float4 switches;float4 maskControl;float4 glow;float4 bleed;float4 tape;float4 tapeNoise;};
struct V{float4 p:SV_POSITION;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float hash(float2 p){return frac(sin(dot(p,float2(12.9898,78.233)))*43758.5453);}
float noise(float x){float f=frac(x);return lerp(hash(floor(x).xx),hash((floor(x)+1).xx),f*f*(3-2*f));}
// FYLM spatial YIQ approximation: separate luma and chroma bandwidths.
float3 toYIQ(float3 c){return float3(dot(c,float3(.299,.587,.114)),dot(c,float3(.5959,-.2746,-.3213)),dot(c,float3(.2115,-.5227,.3112)));}
float3 fromYIQ(float3 c){return float3(c.x+.956*c.y+.619*c.z,c.x-.272*c.y-.647*c.z,c.x-1.106*c.y+1.703*c.z);}
float3 tapeColour(float2 uv){
 float2 offset=float2(geometry.w/720,0);float3 iq=toYIQ(source.Sample(sample0,uv+offset).rgb)*.4026;
 float y=toYIQ(source.Sample(sample0,uv).rgb).x*.4026;
 for(int i=1;i<=2;i++){float w=i==1?.2442:.0545;float dx=(bleed.x+bleed.y)*i/720;float dy=bleed.x*i/720;
  iq+=(toYIQ(source.Sample(sample0,uv+offset+float2(dx,0)).rgb)+toYIQ(source.Sample(sample0,uv+offset-float2(dx,0)).rgb))*w;
  y+=(toYIQ(source.Sample(sample0,uv+float2(dy,0)).rgb).x+toYIQ(source.Sample(sample0,uv-float2(dy,0)).rgb).x)*w;
 }iq.x=y;
 float frame=floor(motion.x*25),scanRow=floor(uv.y*480),cell=floor(uv.x*90);
 iq.yz+=(float2(hash(float2(cell+scanRow*.17,frame+tapeNoise.x)),hash(float2(cell*.73-scanRow*.11,frame+41+tapeNoise.x)))-.5)*tape.y*.16;
 return fromYIQ(iq);
}
float4 ps(V v):SV_TARGET{
 float2 uv=v.uv,p=uv*2-1;
 if(switches.y>0)uv=(p*(1+geometry.z*dot(p,p))+1)*.5;
 if(any(uv<0)||any(uv>1))return 0;
 float time=motion.x+(tape.x>0?tapeNoise.x*.013:0);float2 screenUV=uv;bool vhs=switches.x>0&&tape.w>0;
 if(vhs){
  uv.x+=((noise(uv.y*motion.z+time*2)*2-1)+.4*(noise(uv.y*motion.z*3-time*4)*2-1))*motion.y*tape.w;
  float bottom=tracking.x>0?smoothstep(1-tracking.x,1,uv.y):0;
  uv.x+=(hash(float2(uv.y*200,time*10))-.5)*tracking.y*bottom*tape.w;
  if(tape.x>0&&tape.z>0){float scanRow=.985+.003*sin(floor(time*25)*1.7+tapeNoise.x);float band=smoothstep(scanRow-.003,scanRow,uv.y);uv.x+=(hash(float2(floor(time*25),tapeNoise.x))-.5)*tape.z*.035*band*tape.w;}
 }
 float offset=vhs?geometry.w/geometry.x:0;
 float2 chroma=float2(offset,0)+(switches.y>0?(uv-.5)*dot(uv-.5,uv-.5)*maskControl.w:float2(0,0));
 float4 mid=source.Sample(sample0,uv);
 float3 c=float3(source.Sample(sample0,uv+chroma).r,mid.g,source.Sample(sample0,uv-chroma).b);
 if(vhs&&tape.x>0)c=tapeColour(uv);
 else if(vhs){
  float3 blurred=c*.4026;
  for(int tap=1;tap<=2;tap++){
   float w=tap==1?.2442:.0545;float dx=(bleed.x+bleed.y)*tap/geometry.x;float gy=bleed.x*tap/geometry.x;
   blurred+=float3(source.Sample(sample0,uv+float2(dx,0)+chroma).r,source.Sample(sample0,uv+float2(gy,0)).g,source.Sample(sample0,uv+float2(dx,0)-chroma).b)*w;
   blurred+=float3(source.Sample(sample0,uv-float2(dx,0)+chroma).r,source.Sample(sample0,uv-float2(gy,0)).g,source.Sample(sample0,uv-float2(dx,0)-chroma).b)*w;
  }c=blurred;
 }
 if(vhs){
  float frame=floor(time*25);float bottom=tracking.x>0?smoothstep(1-tracking.x,1,uv.y):0;
  c=lerp(c,hash(uv+time).xxx,bottom*tracking.z*hash(float2(uv.y*200,time*10)));
  float band=step(1-streak.x,hash(float2(floor(uv.y/max(.0001,streak.z)),frame)));
  c=lerp(c,c.gbr,band*streak.y);
  float drop=step(1-dropout.x,hash(float2(floor(uv.y/max(.0001,dropout.z)),frame+37)));
  c=lerp(c,hash(uv+frame).xxx,drop*dropout.y);
  float3 n=float3(hash(uv*float2(400,300)+time),hash(uv*float2(400,300)+time+1),hash(uv*float2(400,300)+time+2));
  c+=(lerp(n.ggg,n,signal.z)-.5)*signal.y;
 }
 if(vhs&&tape.w<1)c=lerp(source.Sample(sample0,screenUV).rgb,c,tape.w);
 // Scanlines belong to the display stage and remain available in CRT-only mode.
 if(signal.x>0&&(tape.x<.5||switches.y>0))c*=lerp(1-signal.w,1,pow(saturate((sin(v.uv.y*signal.x*6.2831853)+1)*.5),glow.w));
 // Display colour controls apply to the whole display stage: CRT keeps its original order
 // (after halation, before mask), and VHS-only mode now gets the same grading.
 if(switches.y>0){
  if(glow.x>0){float2 pixel=glow.y/geometry.xy;float3 bloom=0;
   bloom+=max(source.Sample(sample0,uv+float2(pixel.x,0)).rgb-.55,0);
   bloom+=max(source.Sample(sample0,uv-float2(pixel.x,0)).rgb-.55,0);
   bloom+=max(source.Sample(sample0,uv+float2(0,pixel.y)).rgb-.55,0);
   bloom+=max(source.Sample(sample0,uv-float2(0,pixel.y)).rgb-.55,0);c+=bloom*glow.x*.25;}
  c=(c+screen.x-.5)*screen.y+.5;
  c=lerp(dot(c,float3(.299,.587,.114)).xxx,c,screen.z);
  float phase=uv.x*maskControl.x*6.2831853;
  float row=floor(uv.y*geometry.y);
  if(maskControl.z>1.5)phase+=fmod(row,2)*3.14159265;
  float3 mask=saturate(sin(float3(phase,phase-2.09439,phase-4.18879))*2);
  if(maskControl.z>2.5)mask*=lerp(.5,1,step(.2,frac(uv.y*maskControl.x*.5)));
  float coverage=saturate(geometry.x/max(1,maskControl.x)/3);c*=lerp(1,mask,maskControl.y*coverage);
  c*=1-glow.z*smoothstep(.45,1,length(uv-.5)*1.414);
 }
 else{
  c=(c+screen.x-.5)*screen.y+.5;
  c=lerp(dot(c,float3(.299,.587,.114)).xxx,c,screen.z);
 }
 if(switches.z>0)c=saturate(c);
 return float4(c,mid.a);
})";
class Backend final:public IRenderBackend {
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 ComPtr<ID3D11VertexShader> vertex;ComPtr<ID3D11PixelShader> pixel,tapePixel;
 ComPtr<ID3D11SamplerState> sampler;ComPtr<ID3D11Buffer> constants;
 ComPtr<ID3D11Texture2D> input,target,staging;ComPtr<ID3D11ShaderResourceView> view;ComPtr<ID3D11RenderTargetView> output;
 retro::Size size{};std::string deviceName;
public:
 Backend(){D3D_FEATURE_LEVEL level;
  auto h=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context);
  if(FAILED(h)){checked(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));deviceName="D3D11 WARP";}else deviceName="D3D11 hardware";
  ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC description{};if(SUCCEEDED(device.As(&dxgi))&&SUCCEEDED(dxgi->GetAdapter(&adapter))&&SUCCEEDED(adapter->GetDesc(&description))){char name[512]{};WideCharToMultiByte(CP_UTF8,0,description.Description,-1,name,sizeof(name),nullptr,nullptr);deviceName+=" / "+std::string(name);}
  for(int stage=0;stage<2;++stage){ComPtr<ID3DBlob> code,errors;auto compiled=D3DCompile(detail::LegacyShader,std::strlen(detail::LegacyShader),"FYLM built-in",nullptr,nullptr,stage?"ps":"vs",stage?"ps_5_0":"vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);if(FAILED(compiled)&&errors)throw std::runtime_error(std::string(static_cast<const char*>(errors->GetBufferPointer()),errors->GetBufferSize()));checked(compiled);
   if(stage)checked(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&pixel));else checked(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vertex));}
  D3D11_SAMPLER_DESC s{};s.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;s.MaxLOD=D3D11_FLOAT32_MAX;checked(device->CreateSamplerState(&s,&sampler));
  D3D11_BUFFER_DESC b{};b.ByteWidth=208;b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;checked(device->CreateBuffer(&b,nullptr,&constants));
 }
 const char* name()const noexcept override{return deviceName.c_str();}
 void releaseFrame()noexcept override{context->ClearState();view.Reset();output.Reset();input.Reset();target.Reset();staging.Reset();size={};context->Flush();}
 retro::RenderOutput render(const retro::RenderRequest& r,const SignalSettings& settings)override{
  if(r.size.width<=0||r.size.height<=0||r.size.width>16384||r.size.height>16384||r.input.size()!=std::size_t(r.size.width)*r.size.height*4||!r.timeScale)return {false,"retro.error.frame",{}};
  if(r.input.size()>r.memoryBudget/12)return {false,"retro.error.memory",{}};
  if(!r.controls.retroEnabled||(!r.controls.crt.vhsEnabled&&!r.controls.crt.crtEnabled))return {true,{},r.input};
  bool legacy=!r.controls.crt.vhsEnabled||settings.strength==0||(settings.model==0&&settings.strength==1);
  if(!legacy&&!tapePixel){ComPtr<ID3DBlob> code,errors;auto status=D3DCompile(shader,std::strlen(shader),"FYLM YIQ tape",nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);if(FAILED(status)&&errors)throw std::runtime_error(std::string(static_cast<const char*>(errors->GetBufferPointer()),errors->GetBufferSize()));checked(status);checked(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&tapePixel));}
  if(size.width!=r.size.width||size.height!=r.size.height){context->ClearState();view.Reset();output.Reset();input.Reset();target.Reset();staging.Reset();
   D3D11_TEXTURE2D_DESC t{};t.Width=r.size.width;t.Height=r.size.height;t.MipLevels=t.ArraySize=1;t.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   checked(device->CreateTexture2D(&t,nullptr,&input));checked(device->CreateShaderResourceView(input.Get(),nullptr,&view));
   t.BindFlags=D3D11_BIND_RENDER_TARGET;checked(device->CreateTexture2D(&t,nullptr,&target));checked(device->CreateRenderTargetView(target.Get(),nullptr,&output));
   t.BindFlags=0;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;checked(device->CreateTexture2D(&t,nullptr,&staging));size=r.size;
  }
  const auto& p=r.controls.crt;float c[52]={float(size.width),float(size.height),p.curvature,p.chromaShiftPixels,
   float((r.controls.animateTime?double(r.timeValue)/r.timeScale*r.controls.timeRate:0)+r.controls.timeOffset),p.wobbleIntensity,p.wobbleFrequency,0,
   (settings.model==1&&!p.crtEnabled?0:p.scanlineCount),p.noiseAmount,p.noiseColor,p.scanlineOpacity,p.trackingHeight,p.trackingDistortion,p.trackingNoise,0,
   p.streakFrequency,p.streakIntensity,p.streakHeight,0,p.dropoutFrequency,p.dropoutIntensity,p.dropoutHeight,0,
   p.brightness,p.contrast,p.saturation,0,float(settings.strength==0?0:p.vhsEnabled),float(p.crtEnabled),float(p.clampOutput),0,
   p.maskDensity,p.maskIntensity,float(p.maskType),p.convergence,p.halationIntensity,p.halationRadius,p.vignetteIntensity,p.scanlineSharpness,
   p.luminanceBlur,p.chromaBleed,p.cornerRadius,p.borderSmoothness,
   float(settings.model),settings.chromaNoise,settings.headSwitch,settings.strength,float(settings.seed),0,0,0};
  context->UpdateSubresource(input.Get(),0,nullptr,r.input.data(),size.width*16,0);context->UpdateSubresource(constants.Get(),0,nullptr,c,0,0);
  auto srv=view.Get();auto rt=output.Get();auto cb=constants.Get();auto ss=sampler.Get();
  D3D11_VIEWPORT viewport{0,0,float(size.width),float(size.height),0,1};context->RSSetViewports(1,&viewport);context->OMSetRenderTargets(1,&rt,nullptr);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vertex.Get(),nullptr,0);context->PSSetShader(legacy?pixel.Get():tapePixel.Get(),nullptr,0);context->PSSetShaderResources(0,1,&srv);context->PSSetSamplers(0,1,&ss);context->PSSetConstantBuffers(0,1,&cb);context->Draw(3,0);
  ID3D11ShaderResourceView* nullView=nullptr;context->PSSetShaderResources(0,1,&nullView);context->CopyResource(staging.Get(),target.Get());
  D3D11_MAPPED_SUBRESOURCE mapped{};checked(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
  retro::RenderOutput result;try{result.pixels.resize(r.input.size());for(int y=0;y<size.height;++y)std::memcpy(result.pixels.data()+std::size_t(y)*size.width*4,static_cast<char*>(mapped.pData)+std::size_t(y)*mapped.RowPitch,size.width*16);}catch(...){context->Unmap(staging.Get(),0);throw;}
  context->Unmap(staging.Get(),0);result.ok=true;return result;
 }
};
}
std::unique_ptr<IRenderBackend> makeD3D11Backend(){return std::make_unique<Backend>();}
}
