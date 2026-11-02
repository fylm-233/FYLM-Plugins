// Append after crt_vhs.glsl and dither.glsl; prefix #version 330 core in the assembler.
// Compile variants with RETRO_PASS = 0(copy/resize), 1(dither), 2(VHS), 3(CRT), 4(convenience combined).
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uInput;
uniform sampler2D uPalette;
uniform usampler2D uNoiseRanks;
uniform vec2 uResolution;
uniform ivec2 uWorkOrigin;
uniform float uTime;
uniform CRTVHSParams uCrt;
uniform DitherParams uDither;
uniform sampler2D uOriginalAlpha;
uniform float uToneContrast;
uniform float uToneMidtones;
uniform float uToneHighlights;
void main() {
#if RETRO_PASS == 1
    ivec2 pixel=ivec2(floor(vUV*uResolution))+uWorkOrigin;
    fragColor=processDither(uInput,uPalette,uNoiseRanks,vUV,pixel,uDither);
#elif RETRO_PASS == 2
    fragColor=processVHS(uInput,vUV,uResolution,uTime,uCrt);
#elif RETRO_PASS == 3
    fragColor=processCRT(uInput,vUV,uResolution,uTime,uCrt);
#elif RETRO_PASS == 4
    fragColor=processCRTVHS(uInput,vUV,uResolution,uTime,uCrt);
#elif RETRO_PASS == 5
    vec4 s=roundEven(clamp(texture(uInput,vUV),0.0,1.0)*255.0);
    float cf=1.0+clamp(uToneContrast,-100.0,100.0)/100.0;
    float gm=clamp(uToneMidtones,0.5,2.0),ha=clamp(uToneHighlights,0.0,100.0)/100.0;
    if(abs(cf-1.0)>=0.001||abs(gm-1.0)>=0.001||ha>=0.001){
        float L=dot(s.rgb,vec3(0.299,0.587,0.114));
        float l=pow(clamp(0.5+(L/255.0-0.5)*cf,0.0,1.0),gm);
        float t=clamp((l-0.6)/0.4,0.0,1.0);t=t*t*(3.0-2.0*t);
        if(ha>0.0)l=mix(l,pow(l,1.0+1.5*ha),t);
        s.rgb=roundEven(clamp(s.rgb*(L>0.0001?l*255.0/L:0.0),0.0,255.0));
    }
    fragColor=s/255.0;
#elif RETRO_PASS == 6
    fragColor=texture(uInput,vUV);
    fragColor.a=texture(uOriginalAlpha,vUV).a;
#else
    fragColor=texture(uInput,vUV);
#endif
}
