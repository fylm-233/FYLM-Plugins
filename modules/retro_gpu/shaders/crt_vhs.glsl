// Adapted from punikonta/godot-shader-crt-vhs, MIT, Marcel Jovic.
// Reference: 6ba87f95c152ec023fc07c4f77e87bdd70783afa. See third_party/LICENSE.crt-vhs.txt.
// GLSL 330 core module: no Godot built-ins, entry point, uniforms or nonstandard #include.
#ifndef RETRO_CRT_VHS_GLSL
#define RETRO_CRT_VHS_GLSL
const float RETRO_PI = 3.14159265358979323846;
struct CRTVHSParams {
    int vhsEnabled; int crtEnabled; int clampOutput;
    float curvature; // NEW: not present in the upstream shaders.
    float scanlineCount; float scanlineOpacity;
    float luminanceBlur; float chromaBleed; float chromaShiftPixels;
    float trackingHeight; float trackingDistortion; float trackingNoise;
    float wobbleIntensity; float wobbleFrequency; // normalized horizontal UV and cycles
    float noiseAmount; float noiseColor;
    float streakFrequency; float streakIntensity; float streakHeight;
    float dropoutFrequency; float dropoutIntensity; float dropoutHeight;
    float maskDensity; float maskIntensity; float convergence;
    float halationIntensity; float halationRadius;
    float brightness; float contrast; float saturation;
    float vignetteIntensity; float vignetteSmoothness;
    float cornerRadius; float borderX; float borderY; float borderSmoothness;
};
float retroRandom(vec2 p) { return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123); }
float retroSmoothNoise(float x) {
    float i=floor(x), f=fract(x), u=f*f*(3.0-2.0*f);
    return mix(retroRandom(vec2(i)),retroRandom(vec2(i+1.0)),u);
}
// NEW inverse sampling warp; strength=0 is identity. Caller supplies full-frame UV.
vec2 applyCurvature(vec2 uv,float strength) {
    vec2 p=uv*2.0-1.0;
    return (p*(1.0+strength*dot(p,p))+1.0)*0.5;
}
vec3 applyScanlines(vec3 color,vec2 uv,float density,float opacity) {
    if(density<=0.0||opacity<=0.0)return color;
    float wave=sin(uv.y*density*RETRO_PI*2.0)*0.5+0.5;
    return color*mix(1.0-opacity,1.0,wave);
}
// offset is normalized horizontal UV; pixel controls must be divided by texture width.
vec3 applyChromaShift(sampler2D tex,vec2 uv,float offset) {
    vec2 d=vec2(offset,0.0);
    return vec3(texture(tex,uv+d).r,texture(tex,uv).g,texture(tex,uv-d).b);
}
vec2 applyJitter(vec2 uv,float time,float intensity,float frequency) {
    float wow=retroSmoothNoise(uv.y*frequency+time*2.0)*2.0-1.0;
    float flutter=retroSmoothNoise(uv.y*frequency*3.0-time*4.0)*2.0-1.0;
    return uv+vec2((wow+flutter*0.4)*intensity,0.0);
}
vec2 applyJitter(vec2 uv,float time,float intensity) { return applyJitter(uv,time,intensity,4.0); }
vec3 applyNoise(vec3 color,vec2 uv,float time,float amount,float colorMix) {
    if(amount<=0.0)return color;
    vec2 t=vec2(retroRandom(vec2(time,0.0)),retroRandom(vec2(time,1.0)));
    vec2 n=uv*vec2(400.0,300.0);
    vec3 rgb=vec3(retroRandom(n+t),retroRandom(n+t*1.1),retroRandom(n+t*1.2));
    vec3 noise=mix(vec3(rgb.g),rgb,colorMix);
    float luma=dot(color,vec3(0.299,0.587,0.114));
    return color+(noise-mix(0.0,0.5,luma))*mix(amount*2.0,amount,luma);
}
vec3 applyNoise(vec3 color,vec2 uv,float time,float amount) { return applyNoise(color,uv,time,amount,0.3); }
vec3 retroColorBleed(sampler2D tex,vec2 uv,vec2 resolution,CRTVHSParams p) {
    float shift=p.chromaShiftPixels/resolution.x;
    if(p.luminanceBlur==0.0&&p.chromaBleed==0.0)return applyChromaShift(tex,uv,shift);
    vec2 blur=vec2(p.luminanceBlur/resolution.x,0.0);
    vec2 bleed=vec2((p.luminanceBlur+p.chromaBleed)/resolution.x,0.0);
    vec3 c=applyChromaShift(tex,uv,shift)*0.4026;
    for(int i=1;i<=2;i++) {
        float w=i==1?0.2442:0.0545;
        vec2 d=bleed*float(i),g=blur*float(i),s=vec2(shift,0.0);
        c+=vec3(texture(tex,uv+d+s).r,texture(tex,uv+g).g,texture(tex,uv+d-s).b)*w;
        c+=vec3(texture(tex,uv-d+s).r,texture(tex,uv-g).g,texture(tex,uv-d-s).b)*w;
    }
    return c;
}
vec4 processVHS(sampler2D tex,vec2 uv,vec2 resolution,float time,CRTVHSParams p) {
    if(p.vhsEnabled==0)return texture(tex,uv);
    float bottom=p.trackingHeight>0.0?smoothstep(1.0-p.trackingHeight,1.0,uv.y):0.0;
    float tear=retroRandom(vec2(uv.y*200.0,time*10.0)),skew=retroRandom(vec2(time*2.0,0.0))-0.5;
    vec2 base=applyJitter(uv,time,p.wobbleIntensity,p.wobbleFrequency);
    base.x+=((tear-0.5)*0.5+skew*0.5)*p.trackingDistortion*bottom;
    vec3 c=retroColorBleed(tex,base,resolution,p);
    vec2 t=vec2(retroRandom(vec2(time,0.0)),retroRandom(vec2(time,1.0)));
    c=mix(c,vec3(retroRandom(base+t)),bottom*p.trackingNoise*tear);
    float frame=floor(time*25.0);
    vec2 frameOffset=vec2(retroRandom(vec2(frame,0.0)),retroRandom(vec2(frame,1.0)));
    vec2 grid=vec2(floor(uv.x),floor(uv.y/max(0.0001,p.streakHeight)));
    float streak=step(1.0-p.streakFrequency,retroRandom(grid+frameOffset*50.0));
    vec3 shifted=mix(c.gbr,c.brg,step(0.5,retroRandom(grid+frame)));
    c=mix(c,shifted,streak*p.streakIntensity);
    float dropout=step(1.0-p.dropoutFrequency,retroRandom(vec2(floor(uv.y/max(0.0001,p.dropoutHeight)),frameOffset.x)));
    c=mix(c,vec3(retroRandom(uv+frameOffset*100.0)),dropout*p.dropoutIntensity);
    c=applyNoise(c,base,time,p.noiseAmount,p.noiseColor);
    if(p.clampOutput!=0)c=clamp(c,0.0,1.0);
    c=applyScanlines(c,uv,p.scanlineCount,p.scanlineOpacity);
    return vec4(c,texture(tex,base).a);
}
// CRT can sample the previous VHS texture (production) or evaluate VHS procedurally (convenience).
vec4 retroCRTInput(sampler2D tex,vec2 uv,vec2 res,float time,CRTVHSParams p,bool evaluateVHS) {
    return evaluateVHS?processVHS(tex,uv,res,time,p):texture(tex,uv);
}
vec4 retroCRT(sampler2D tex,vec2 uv,vec2 resolution,float time,CRTVHSParams p,bool evaluateVHS) {
    if(p.crtEnabled==0)return retroCRTInput(tex,uv,resolution,time,p,evaluateVHS);
    vec2 base=applyCurvature(uv,p.curvature);
    if(any(lessThan(base,vec2(0.0)))||any(greaterThan(base,vec2(1.0))))return vec4(0.0);
    vec2 center=base-0.5,offset=center*dot(center,center)*p.convergence;
    vec4 middle=retroCRTInput(tex,base,resolution,time,p,evaluateVHS);
    vec3 c=vec3(retroCRTInput(tex,base+offset,resolution,time,p,evaluateVHS).r,
                middle.g,retroCRTInput(tex,base-offset,resolution,time,p,evaluateVHS).b);
    if(p.halationIntensity>0.0) {
        vec2 d=p.halationRadius/resolution;
        vec3 bloom=c*0.382928;
        bloom+=retroCRTInput(tex,base+vec2(d.x,0),resolution,time,p,evaluateVHS).rgb*0.241732;
        bloom+=retroCRTInput(tex,base-vec2(d.x,0),resolution,time,p,evaluateVHS).rgb*0.241732;
        bloom+=retroCRTInput(tex,base+vec2(0,d.y),resolution,time,p,evaluateVHS).rgb*0.060598;
        bloom+=retroCRTInput(tex,base-vec2(0,d.y),resolution,time,p,evaluateVHS).rgb*0.060598;
        c+=bloom*p.halationIntensity;
    }
    c=(c+p.brightness-0.5)*p.contrast+0.5;
    c=mix(vec3(dot(c,vec3(0.299,0.587,0.114))),c,p.saturation);
    if(p.clampOutput!=0)c=clamp(c,0.0,1.0);
    float phase=base.x*p.maskDensity*RETRO_PI*2.0;
    vec3 mask=clamp(sin(vec3(phase,phase-2.09439,phase-4.18879))*2.0,0.0,1.0);
    c*=mix(vec3(1),mask,p.maskIntensity);
    float v=p.vignetteSmoothness>0.0?1.0-smoothstep(1.0-p.vignetteSmoothness,1.0,length(center)*1.414):1.0;
    c=mix(c,c*v,p.vignetteIntensity);
    vec2 aspect=vec2(resolution.x/resolution.y,1.0);
    vec2 q=abs(center*aspect)-(vec2(0.5)-vec2(p.borderX,p.borderY))*aspect+p.cornerRadius;
    float edge=min(max(q.x,q.y),0.0)+length(max(q,0.0))-p.cornerRadius;
    float border=p.borderSmoothness>0.0?1.0-smoothstep(0.0,p.borderSmoothness,edge):1.0-step(0.0,edge);
    return vec4(c*border,middle.a); // Upstream black border preserves alpha.
}
vec4 processCRT(sampler2D tex,vec2 uv,vec2 resolution,float time,CRTVHSParams params) {
    return retroCRT(tex,uv,resolution,time,params,false);
}
// Requested complete entry. Production uses processVHS then processCRT in two shared-context passes.
// This convenience variant resamples procedural VHS repeatedly and is not pixel-identical to a filtered intermediate texture.
vec4 processCRTVHS(sampler2D tex,vec2 uv,vec2 resolution,float time,CRTVHSParams params) {
    return retroCRT(tex,uv,resolution,time,params,true);
}
#endif
