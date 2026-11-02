#pragma once
// Frozen 0.11 shader for exact legacy rendering compatibility.
namespace fylm::crt::detail {inline constexpr const char* LegacyShader=R"FYLM(
Texture2D source:register(t0);SamplerState sample0:register(s0);
cbuffer Frame:register(b0){float4 geometry;float4 motion;float4 signal;float4 tracking;float4 streak;float4 dropout;float4 screen;float4 switches;float4 maskControl;float4 glow;float4 bleed;};
struct V{float4 p:SV_POSITION;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.p=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float hash(float2 p){return frac(sin(dot(p,float2(12.9898,78.233)))*43758.5453);}
float noise(float x){float f=frac(x);return lerp(hash(floor(x).xx),hash((floor(x)+1).xx),f*f*(3-2*f));}
float4 ps(V v):SV_TARGET{
 float2 uv=v.uv,p=uv*2-1;
 if(switches.y>0)uv=(p*(1+geometry.z*dot(p,p))+1)*.5;
 if(any(uv<0)||any(uv>1))return 0;
 float time=motion.x;
 if(switches.x>0){
  uv.x+=((noise(uv.y*motion.z+time*2)*2-1)+.4*(noise(uv.y*motion.z*3-time*4)*2-1))*motion.y;
  float bottom=tracking.x>0?smoothstep(1-tracking.x,1,uv.y):0;
  uv.x+=(hash(float2(uv.y*200,time*10))-.5)*tracking.y*bottom;
 }
 float offset=switches.x>0?geometry.w/geometry.x:0;
 float2 chroma=float2(offset,0)+(switches.y>0?(uv-.5)*dot(uv-.5,uv-.5)*maskControl.w:float2(0,0));
 float4 mid=source.Sample(sample0,uv);
 float3 c=float3(source.Sample(sample0,uv+chroma).r,mid.g,source.Sample(sample0,uv-chroma).b);
 if(switches.x>0){
  float3 blurred=c*.4026;
  for(int tap=1;tap<=2;tap++){
   float w=tap==1?.2442:.0545;float dx=(bleed.x+bleed.y)*tap/geometry.x;float gy=bleed.x*tap/geometry.x;
   blurred+=float3(source.Sample(sample0,uv+float2(dx,0)+chroma).r,source.Sample(sample0,uv+float2(gy,0)).g,source.Sample(sample0,uv+float2(dx,0)-chroma).b)*w;
   blurred+=float3(source.Sample(sample0,uv-float2(dx,0)+chroma).r,source.Sample(sample0,uv-float2(gy,0)).g,source.Sample(sample0,uv-float2(dx,0)-chroma).b)*w;
  }c=blurred;
 }
 if(switches.x>0){
  float frame=floor(time*25);float bottom=tracking.x>0?smoothstep(1-tracking.x,1,uv.y):0;
  c=lerp(c,hash(uv+time).xxx,bottom*tracking.z*hash(float2(uv.y*200,time*10)));
  float band=step(1-streak.x,hash(float2(floor(uv.y/max(.0001,streak.z)),frame)));
  c=lerp(c,c.gbr,band*streak.y);
  float drop=step(1-dropout.x,hash(float2(floor(uv.y/max(.0001,dropout.z)),frame+37)));
  c=lerp(c,hash(uv+frame).xxx,drop*dropout.y);
  float3 n=float3(hash(uv*float2(400,300)+time),hash(uv*float2(400,300)+time+1),hash(uv*float2(400,300)+time+2));
  c+=(lerp(n.ggg,n,signal.z)-.5)*signal.y;
 }
 // Scanlines belong to the display stage and remain available in CRT-only mode.
 if(signal.x>0)c*=lerp(1-signal.w,1,pow(saturate((sin(v.uv.y*signal.x*6.2831853)+1)*.5),glow.w));
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
})FYLM";}
