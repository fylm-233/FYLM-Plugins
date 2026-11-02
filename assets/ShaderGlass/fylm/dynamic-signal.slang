#version 450
// SPDX-License-Identifier: GPL-3.0-or-later
// FYLM original spatial analog-signal approximation. No history/feedback buffers.
layout(std140,set=0,binding=0) uniform UBO {
 mat4 MVP;vec4 SourceSize;vec4 OutputSize;uint FrameCount;float FrameTime;
} global;
layout(push_constant) uniform Push {
 float FYLM_JITTER;float FYLM_CHROMA;float FYLM_BLEED;float FYLM_LUMA_NOISE;
 float FYLM_CHROMA_NOISE;float FYLM_INTERFERENCE;float FYLM_FLICKER;float FYLM_WOW;
 float FYLM_HEAD;float FYLM_DROPOUT;float FYLM_LINES;float FYLM_SPEED;
 float FYLM_SEED;float FYLM_ECHO;float FYLM_ECHO_OFFSET;float FYLM_HUE_DRIFT;
} params;
#pragma parameter FYLM_JITTER "Horizontal jitter (720 reference px)" 0.2 0.0 8.0 0.05
#pragma parameter FYLM_CHROMA "Chroma delay (720 reference px)" 0.3 -8.0 8.0 0.05
#pragma parameter FYLM_BLEED "Chroma bleed (720 reference px)" 0.8 0.0 12.0 0.1
#pragma parameter FYLM_LUMA_NOISE "Luma noise" 0.007 0.0 0.2 0.001
#pragma parameter FYLM_CHROMA_NOISE "Chroma noise" 0.006 0.0 0.2 0.001
#pragma parameter FYLM_INTERFERENCE "Moving interference bars" 0.004 0.0 0.2 0.001
#pragma parameter FYLM_FLICKER "Luminance flicker" 0.008 0.0 0.2 0.001
#pragma parameter FYLM_WOW "Slow horizontal drift (720 reference px)" 0.2 0.0 8.0 0.05
#pragma parameter FYLM_HEAD "Head switching" 0.0 0.0 1.0 0.01
#pragma parameter FYLM_DROPOUT "Line dropout probability" 0.0 0.0 0.1 0.001
#pragma parameter FYLM_LINES "Signal/noise line density" 480.0 120.0 1080.0 1.0
#pragma parameter FYLM_SPEED "Local motion speed" 1.0 0.0 4.0 0.01
#pragma parameter FYLM_SEED "Deterministic noise seed" 3.0 0.0 65535.0 1.0
#pragma parameter FYLM_ECHO "Spatial signal echo" 0.01 0.0 0.3 0.005
#pragma parameter FYLM_ECHO_OFFSET "Echo distance (720 reference px)" 2.0 0.0 20.0 0.1
#pragma parameter FYLM_HUE_DRIFT "Line-correlated hue drift" 0.002 0.0 0.2 0.001
#pragma stage vertex
layout(location=0) in vec4 Position;layout(location=1) in vec2 TexCoord;
layout(location=0) out vec2 uv;
void main(){gl_Position=global.MVP*Position;uv=TexCoord;}
#pragma stage fragment
layout(location=0) in vec2 uv;layout(location=0) out vec4 FragColor;
layout(set=0,binding=2) uniform sampler2D Source;
float random2(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}
float smoothNoise(float x){float f=fract(x);return mix(random2(vec2(floor(x))),random2(vec2(floor(x)+1.0)),f*f*(3.0-2.0*f));}
vec3 yiq(vec3 c){return vec3(dot(c,vec3(.299,.587,.114)),dot(c,vec3(.5959,-.2746,-.3213)),dot(c,vec3(.2115,-.5227,.3112)));}
vec3 rgb(vec3 c){return vec3(c.x+.956*c.y+.619*c.z,c.x-.272*c.y-.647*c.z,c.x-1.106*c.y+1.703*c.z);}
void main(){
 float time=global.FrameTime*params.FYLM_SPEED,frame=floor(time*30.0),seed=floor(params.FYLM_SEED+.5);
 float scanRow=floor(uv.y*floor(params.FYLM_LINES+.5));
 float drift=(smoothNoise(uv.y*4.0+time*.8+seed*.01)-.5)*params.FYLM_WOW;
 drift+=(smoothNoise(scanRow*.31+frame*.77+seed)-.5)*params.FYLM_JITTER;
 float head=smoothstep(.979+.003*sin(time*6.0),.992,uv.y);
 drift+=head*(random2(vec2(frame,seed))-.5)*params.FYLM_HEAD*20.0;
 vec2 sampleUV=uv+vec2(drift/720.0,0.0);
 vec4 original=texture(Source,sampleUV);vec2 delay=vec2(params.FYLM_CHROMA/720.0,0.0);
 vec3 signal=yiq(texture(Source,sampleUV+delay).rgb)*.4026;
 for(int tap=1;tap<=2;tap++){
  float weight=tap==1?.2442:.0545;vec2 d=vec2(params.FYLM_BLEED*float(tap)/720.0,0.0);
  signal+=(yiq(texture(Source,sampleUV+delay+d).rgb)+yiq(texture(Source,sampleUV+delay-d).rgb))*weight;
 }
 signal.x=yiq(original.rgb).x;
 signal.x+=(random2(vec2(floor(uv.x*720.0)+seed,scanRow+frame*.17))-.5)*params.FYLM_LUMA_NOISE;
 signal.yz+=(vec2(random2(vec2(floor(uv.x*90.0)+seed,scanRow+frame)),random2(vec2(scanRow+seed,frame+7.0)))-.5)*params.FYLM_CHROMA_NOISE;
 float angle=(smoothNoise(scanRow*.04+time+seed)-.5)*params.FYLM_HUE_DRIFT;
 signal.yz=mat2(cos(angle),sin(angle),-sin(angle),cos(angle))*signal.yz;
 signal.x+=sin(uv.y*36.0+time*3.0)*params.FYLM_INTERFERENCE;
 signal.x*=1.0+(smoothNoise(time*3.0+seed)-.5)*params.FYLM_FLICKER;
 vec3 color=rgb(signal);
 color=mix(color,texture(Source,sampleUV-vec2(params.FYLM_ECHO_OFFSET/720.0,0.0)).rgb,params.FYLM_ECHO);
 float drop=step(1.0-params.FYLM_DROPOUT,random2(vec2(scanRow+seed,frame+37.0)));
 color=mix(color,vec3(random2(vec2(uv.x*720.0+seed,frame))),drop*.5);
 FragColor=vec4(clamp(color,0.0,1.0),original.a);
}
