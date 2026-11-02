// Port of the user-provided reimuplayer halftone.rs, HALFTONE_AE_PLUGIN_SPEC.md.
#include "dither/halftone.hpp"
#include <algorithm>
#include <cmath>
namespace dither {
namespace {
std::uint8_t byte(float v){return static_cast<std::uint8_t>(std::clamp(std::round(v),0.f,255.f));}
void disk(std::vector<std::uint8_t>& a,int w,int h,float cx,float cy,float r,
          const float* rgb,float alpha,bool screen){
    int x0=std::max(0,int(std::floor(cx-r))),y0=std::max(0,int(std::floor(cy-r)));
    int x1=std::min(w,int(std::ceil(cx+r))),y1=std::min(h,int(std::ceil(cy+r)));
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){
        float dx=float(x)+.5f-cx,dy=float(y)+.5f-cy;
        float coverage=std::clamp(r-std::sqrt(dx*dx+dy*dy)+.5f,0.f,1.f);
        if(coverage<=0)continue;
        float opacity=alpha*coverage;auto i=(std::size_t(y)*w+x)*4;
        for(int c=0;c<3;++c){float dst=float(a[i+c]);
            float blended=screen?255.f-(255.f-dst)*(255.f-rgb[c])/255.f:dst*rgb[c]/255.f;
            a[i+c]=byte(dst+(blended-dst)*opacity);
        }
    }
}
float luma(const std::vector<std::uint8_t>& a,int w,int h,float cx,float cy,float cell){
    int x0=int(std::floor(cx)),y0=int(std::floor(cy));
    int x1=std::min(w,int(std::ceil(cx+cell))),y1=std::min(h,int(std::ceil(cy+cell)));
    float sum=0,n=0;
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){auto i=(std::size_t(y)*w+x)*4;
        float r=float(a[i])/255.f,g=float(a[i+1])/255.f,b=float(a[i+2])/255.f;
        sum+=.2126f*r+.7152f*g+.0722f*b;n+=1.f;
    }
    return n==0?0:std::clamp(sum/n,0.f,1.f);
}
}
RenderResult applyHalftone(std::vector<std::uint8_t>& a,int w,int h,float cell,IProgress* progress) noexcept {
    if(w<0||h<0||std::size_t(w)>SIZE_MAX/std::size_t(std::max(1,h))/4||a.size()!=std::size_t(w)*h*4)
        return {false,"invalid halftone buffer",ErrorCode::InvalidImage};
    if(!std::isfinite(cell)||cell<1.f||cell>16.f)return {false,"invalid halftone pitch",ErrorCode::InvalidParams};
    if(w==0||h==0)return {true,{}};
    const float black[]={0,0,0};
    for(float cy=0;cy<float(h);cy+=cell){
        if(progress&&progress->cancelled())return {false,"cancelled",ErrorCode::Cancelled};
        for(float cx=0;cx<float(w);cx+=cell){
            float r=(1.f-luma(a,w,h,cx,cy,cell))*cell*.42f;
            if(r>.05f)disk(a,w,h,cx+cell*.5f,cy+cell*.5f,r,black,.62f*.55f,false);
        }
    }
    const float colors[][4]={{255,0,120,.5f},{0,180,255,.45f}};
    for(int layer=0;layer<2;++layer){
        const float* color=colors[layer];float offset=layer==0?0:cell;
        for(float cy=offset;cy<float(h);cy+=cell*2.f){
            if(progress&&progress->cancelled())return {false,"cancelled",ErrorCode::Cancelled};
            for(float cx=offset;cx<float(w);cx+=cell*2.f)
                disk(a,w,h,cx+cell,cy+cell,cell*.28f,color,color[3]*.5f,true);
        }
    }
    for(int y=0;y<h;++y){
        if(progress&&y%16==0&&progress->cancelled())return {false,"cancelled",ErrorCode::Cancelled};
        for(int x=0;x<w;++x){auto i=(std::size_t(y)*w+x)*4;
            float r=a[i],g=a[i+1],b=a[i+2],l=.2126f*r+.7152f*g+.0722f*b;
            a[i]=byte((l+(r-l)*.9f-128.f)*1.06f+128.f);
            a[i+1]=byte((l+(g-l)*.9f-128.f)*1.06f+128.f);
            a[i+2]=byte((l+(b-l)*.9f-128.f)*1.06f+128.f);
        }
    }
    return {true,{}};
}
}
namespace dither {
RenderResult applyPrintHalftone(std::vector<std::uint8_t>& a,int w,int h,float pitch,float angle,float gain,IProgress* progress) noexcept {
 if(w<0||h<0||std::size_t(w)>SIZE_MAX/std::size_t(std::max(1,h))/4||a.size()!=std::size_t(w)*h*4)return {false,"invalid print buffer",ErrorCode::InvalidImage};
 if(!std::isfinite(pitch)||pitch<1||pitch>16||!std::isfinite(angle)||angle < -180||angle>180||!std::isfinite(gain)||gain < -50||gain>50)return {false,"invalid print settings",ErrorCode::InvalidParams};
 if(!w||!h)return {true,{}};
 try {
  const auto source=a;float cs[4],sn[4];const float angles[]={15,75,0,45};
  for(int c=0;c<4;++c){float theta=(angles[c]+angle)*.01745329252f;cs[c]=std::cos(theta);sn[c]=std::sin(theta);}
  for(int y=0;y<h;++y){if(progress&&progress->cancelled())return {false,"cancelled",ErrorCode::Cancelled};for(int x=0;x<w;++x){float dots[4];
   for(int c=0;c<4;++c){float u=((x+.5f)*cs[c]+(y+.5f)*sn[c])/pitch,v=(-(x+.5f)*sn[c]+(y+.5f)*cs[c])/pitch;
    float cu=std::floor(u)+.5f,cv=std::floor(v)+.5f;
    int sx=std::clamp(int(std::floor((cu*cs[c]-cv*sn[c])*pitch)),0,w-1),sy=std::clamp(int(std::floor((cu*sn[c]+cv*cs[c])*pitch)),0,h-1);
    auto i=(std::size_t(sy)*w+sx)*4;float cyan=1-source[i]/255.f,magenta=1-source[i+1]/255.f,yellow=1-source[i+2]/255.f,k=std::min({cyan,magenta,yellow});
    float amount=c==3?k:k>=.99999f?0:((c==0?cyan:c==1?magenta:yellow)-k)/(1-k);
    amount=std::clamp(amount+gain*.01f*4*amount*(1-amount),0.f,1.f);
    float dx=u-cu,dy=v-cv,r=.70710678f*std::sqrt(amount);
    dots[c]=amount<=0?0:amount>=1?1:std::clamp((r-std::sqrt(dx*dx+dy*dy))*pitch+.5f,0.f,1.f);
   }
   auto out=(std::size_t(y)*w+x)*4;for(int c=0;c<3;++c)a[out+c]=byte(255*(1-dots[c])*(1-dots[3]));
  }}return {true,{}};
 }catch(const std::bad_alloc&){return {false,"print memory allocation failed",ErrorCode::OutOfMemory};}catch(...){return {false,"print failed",ErrorCode::Internal};}
}
}
