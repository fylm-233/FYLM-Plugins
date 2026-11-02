// Dithering Studio compatible P0 pixel-local paths. Apache-2.0; see project NOTICE.
#ifndef RETRO_DITHER_GLSL
#define RETRO_DITHER_GLSL
struct DitherParams { int enabled; int algorithmId; int paletteSize; int invert; int preserveAlpha; float threshold; };
const int RETRO_BAYER4[16]=int[16](0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5);
const int RETRO_BAYER8[64]=int[64](0,32,8,40,2,34,10,42,48,16,56,24,50,18,58,26,
12,44,4,36,14,46,6,38,60,28,52,20,62,30,54,22,3,35,11,43,1,33,9,41,
51,19,59,27,49,17,57,25,15,47,7,39,13,45,5,37,63,31,55,23,61,29,53,21);
vec4 processDither(sampler2D tex,sampler2D paletteTex,usampler2D noiseRanks,
                   vec2 uv,ivec2 fullWorkPixel,DitherParams p) {
    vec4 source=texture(tex,uv);
    if(p.enabled==0)return source;
    // Host validates IDs; FS(1) has no equivalent independent fragment implementation.
    if(p.algorithmId!=2&&p.algorithmId!=8&&p.algorithmId!=15&&p.algorithmId!=17)return source;
    vec4 s=roundEven(clamp(source,0.0,1.0)*255.0);
    bool pal=p.paletteSize>0;
    if(pal&&p.invert!=0)s.rgb=255.0-s.rgb;
    float g=s.r;vec3 c;
    if(p.algorithmId==15) {
        int choice=g<p.threshold?0:1;
        c=p.paletteSize>=2?texelFetch(paletteTex,ivec2(choice,0),0).rgb:vec3(float(choice));
    } else {
        int size=p.algorithmId==2?4:8;
        ivec2 xy=fullWorkPixel;
        float cell,raw=0.0;
        if(p.algorithmId==17)cell=float(texelFetch(noiseRanks,ivec2(xy.x&63,xy.y&63),0).r)/4095.0;
        else {int index=(xy.y&(size-1))*size+(xy.x&(size-1));raw=float(size==4?RETRO_BAYER4[index]:RETRO_BAYER8[index]);cell=raw/float(size*size-1);}
        if(pal) {
            float base=g/255.0+(p.threshold-128.0)/255.0*2.0;
            if(p.algorithmId==17)base=clamp(base,0.0,1.0);
            float adjusted=base+(cell-0.5)/float(p.paletteSize);
            int index=int(clamp(floor(adjusted*float(p.paletteSize-1)+0.5),0.0,float(p.paletteSize-1)));
            c=texelFetch(paletteTex,ivec2(index,0),0).rgb;
        } else {
            float matrix=p.algorithmId==17?cell*255.0:raw*(255.0/float(size*size));
            c=vec3(g<clamp(matrix+p.threshold-128.0,0.0,255.0)?0.0:1.0);
        }
    }
    if(!pal&&p.invert!=0)c=1.0-c;
    return vec4(c,p.preserveAlpha!=0?s.a/255.0:1.0);
}
#endif
