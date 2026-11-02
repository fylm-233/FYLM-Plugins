#include "pixel_art/core.hpp"
#include "pixel_art/structure.hpp"
#include <algorithm>
#include <cmath>
#include <future>
#include <fstream>
#include <iterator>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pa=pixel_art;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(auto style:{pa::Style::GBA,pa::Style::Modern}){auto palette=pa::stylePalette(style);std::set<int> colors;for(auto c:palette)colors.insert((c.r<<16)|(c.g<<8)|c.b);require(palette.size()==std::size_t(style==pa::Style::GBA?48:96)&&colors.size()==palette.size(),"built-in unique palettes");}
 constexpr int w=72,h=48;std::vector<unsigned char> input(w*h*4,255),area(input.size()),feature(input.size()),again(input.size());
 for(int y=0;y<h;y++)for(int c=0;c<3;c++)input[(y*w+2)*4+c]=0;
 // Separate one-pixel black dots must not turn entire blocks black.
 for(int cy=0;cy<8;cy++)for(int cx=2;cx<12;cx++)for(int c=0;c<3;c++)input[((cy*6+2)*w+cx*6+2)*4+c]=0;
 pa::Params p;p.palette={{0,0,0},{255,255,255}};p.grid.block=6;p.style=pa::Style::GBA;p.outlineStrength=0;
 auto in=pa::ConstImageView{input.data(),w,h,w*4,pa::PixelFormat::RGBA8};auto out=[&](auto& pixels){return pa::ImageView{pixels.data(),w,h,std::size_t(w)*4,pa::PixelFormat::RGBA8};};
 require(pa::render(in,out(area),p).ok,"area render");p.grid.sampling=pa::Sampling::Feature;require(pa::render(in,out(feature),p).ok,"feature render");int areaLines=0,featureLines=0,expandedDots=0;
 for(int y=0;y<h;y++){if(area[(y*w+2)*4]==0)areaLines++;if(feature[(y*w+2)*4]==0)featureLines++;for(int x=12;x<w;x++)if(feature[(y*w+x)*4]==0)expandedDots++;}
 require(featureLines>areaLines&&featureLines==h,"feature preserves annotated line");require(expandedDots==0,"isolated dark dots do not swell into blocks");
 p.featureStrength=0;require(pa::render(in,out(again),p).ok&&again==area,"zero feature equals area");p.featureStrength=.65;
 auto asynchronous=std::async(std::launch::async,[&]{std::vector<unsigned char> v(input.size());require(pa::render(in,out(v),p).ok,"concurrent render");return v;});require(pa::render(in,out(again),p).ok&&again==feature&&asynchronous.get()==feature,"concurrent determinism");
 p.grid.block=3;require(pa::render(in,out(again),p).ok,"small-block feature sampling");for(int y=0;y<h;y++)require(again[(y*w+2)*4]==0,"duplicate sample weights preserve one-third-width line");p.grid.block=6;
 {std::vector<unsigned char> hiddenA(9*9*4,0),hiddenB(hiddenA.size()),resultA(hiddenA.size()),resultB(hiddenA.size());for(int y=0;y<9;y++)for(int x=0;x<9;x++){int i=(y*9+x)*4;if(x%3==1){hiddenA[i]=hiddenB[i]=255;hiddenA[i+3]=hiddenB[i+3]=255;}else{hiddenA[i+1]=255;hiddenB[i+2]=255;}}pa::Params transparent=p;transparent.grid.block=3;transparent.palette={{255,0,0},{0,255,0},{0,0,255}};require(pa::render({hiddenA.data(),9,9,36,pa::PixelFormat::RGBA8},{resultA.data(),9,9,36,pa::PixelFormat::RGBA8},transparent).ok&&pa::render({hiddenB.data(),9,9,36,pa::PixelFormat::RGBA8},{resultB.data(),9,9,36,pa::PixelFormat::RGBA8},transparent).ok&&resultA==resultB,"feature sampling ignores transparent hidden RGB");}
 // Adjacent 8-bit values on a coarse-bin boundary must stay one connected
 // stroke after AE's 16-bpc (32768 scale) input conversion.
 {std::vector<unsigned char> source(6*6*4,255),eight(source.size()),sixteen(source.size());std::vector<float> host16(source.size());
  for(int y=0;y<6;y++)for(int x=0;x<6;x++)for(int c=0;c<3;c++)source[(y*6+x)*4+c]=x==2?102+y%2:200;
  for(std::size_t i=0;i<source.size();i++)host16[i]=float(std::lround(source[i]/255.*32768)/32768.);
  pa::Params boundary=p;boundary.palette={{102,102,102},{200,200,200}};
  require(pa::render({source.data(),6,6,24,pa::PixelFormat::RGBA8},{eight.data(),6,6,24,pa::PixelFormat::RGBA8},boundary).ok&&pa::render({host16.data(),6,6,96,pa::PixelFormat::RGBA32F},{sixteen.data(),6,6,24,pa::PixelFormat::RGBA8},boundary).ok&&eight==sixteen&&eight[0]==102,"feature bin boundary stable across AE bit depths");}
 {std::ifstream file(PIXEL_ART_FIXTURE,std::ios::binary);std::vector<unsigned char> source((std::istreambuf_iterator<char>(file)),{});require(source.size()==384*256*4,"cross-depth fixture");std::vector<float> float8(source.size()),float16(source.size());for(std::size_t i=0;i<source.size();i++){float8[i]=source[i]/255.f;float16[i]=float(std::lround(source[i]/255.*32768)/32768.);}
  pa::Params modern;modern.style=pa::Style::Modern;modern.grid.block=3;modern.grid.sampling=pa::Sampling::Feature;modern.featureStrength=.5;modern.palette=pa::stylePalette(modern.style);std::vector<unsigned char> eight(source.size()),thirtytwo(source.size()),sixteen(source.size());auto destination=[&](auto& buffer){return pa::ImageView{buffer.data(),384,256,384*4,pa::PixelFormat::RGBA8};};
  require(pa::render({source.data(),384,256,384*4,pa::PixelFormat::RGBA8},destination(eight),modern).ok&&pa::render({float8.data(),384,256,384*16,pa::PixelFormat::RGBA32F},destination(thirtytwo),modern).ok&&pa::render({float16.data(),384,256,384*16,pa::PixelFormat::RGBA32F},destination(sixteen),modern).ok,"cross-depth full fixture render");
  int different32=0,different16=0;for(std::size_t i=0;i<source.size();i+=4){if(!std::equal(eight.begin()+i,eight.begin()+i+4,thirtytwo.begin()+i))different32++;if(!std::equal(eight.begin()+i,eight.begin()+i+4,sixteen.begin()+i))different16++;}std::cout<<"Modern fixture float32 differences="<<different32<<"; 32768 conversion differences="<<different16<<'\n';require(eight==thirtytwo,"full fixture float32 source conversion stable");std::ofstream evidence(PIXEL_ART_TEST_OUTPUT,std::ios::binary);require(bool(evidence.write(reinterpret_cast<const char*>(sixteen.data()),std::streamsize(sixteen.size()))),"write host precision evidence");}
 // Direct logical-grid fixtures isolate independent rules and counterexamples.
 constexpr int side=9;std::vector<pa::Lab> colors={pa::oklabFromSrgb({100,100,100}),pa::oklabFromSrgb({108,108,108}),pa::oklabFromSrgb({255,255,255})};std::vector<float> alpha(side*side,1);pa::Params settings;settings.style=pa::Style::Modern;
 std::vector<unsigned short> labels(side*side,0);labels[40]=1;auto original=labels;auto changed=pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels[40]==0&&changed[40],"merge low contrast region");
 settings.mergeThreshold=0;settings.cleanSpeckles=false;settings.cleanSpurs=false;settings.cleanStairs=false;labels=original;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels==original,"all cleanup off is exact label baseline");
 settings.cleanSpeckles=true;labels=original;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels[40]==0,"speckle rule independently effective");
 labels.assign(side*side,0);labels[40]=2;auto high=labels;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels==high,"high contrast highlight protected");
 settings.cleanSpurs=true;labels.assign(side*side,0);for(int y=2;y<7;y++)labels[y*side+4]=1;auto line=labels;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels==line,"one-cell thin line and endpoints protected");
 settings.cleanSpeckles=false;labels=line;labels[4*side+5]=1;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels[4*side+5]==0,"unsupported side spur removed");
 settings.cleanSpurs=false;settings.cleanStairs=true;labels.assign(side*side,0);labels[40]=labels[39]=labels[31]=1;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels[40]==0,"corner rule independently effective");labels.assign(side*side,0);labels[40]=labels[39]=labels[31]=2;auto corner=labels;pa::refineLabels(labels,side,side,colors,alpha,settings);require(labels==corner,"high contrast corner counterexample");settings.cleanStairs=false;
 // Mirror the input to verify that edit decisions do not depend on scan direction.
 settings.mergeThreshold=.042;settings.cleanSpeckles=true;labels=original;labels[13]=1;auto mirrored=labels;for(int y=0;y<side;y++)for(int x=0;x<side;x++)mirrored[y*side+x]=labels[y*side+side-1-x];pa::refineLabels(labels,side,side,colors,alpha,settings);pa::refineLabels(mirrored,side,side,colors,alpha,settings);for(int y=0;y<side;y++)for(int x=0;x<side;x++)require(labels[y*side+x]==mirrored[y*side+side-1-x],"mirror/scan invariance");
 // End-to-end modern outputs remain palette labels; diagnostic masks are explicit.
 std::vector<unsigned char> noise(9*9*4,255),clean(noise.size()),baseline(noise.size()),mask(noise.size());for(int i=0;i<81;i++)for(int c=0;c<3;c++)noise[i*4+c]=i==40?108:100;
 settings.palette={{100,100,100},{108,108,108}};settings.grid.block=1;settings.style=pa::Style::Modern;auto ni=pa::ConstImageView{noise.data(),9,9,36,pa::PixelFormat::RGBA8};auto no=[&](auto& v){return pa::ImageView{v.data(),9,9,36,pa::PixelFormat::RGBA8};};require(pa::render(ni,no(clean),settings).ok,"modern image render");settings.mergeThreshold=0;settings.cleanSpeckles=false;settings.cleanSpurs=false;require(pa::render(ni,no(baseline),settings).ok&&clean!=baseline,"modern cleanup changes low contrast speckle image");settings.mergeThreshold=.042;settings.debugChanges=true;require(pa::render(ni,no(mask),settings).ok&&mask[40*4]==255&&mask[40*4+3]==255&&mask[3]==0,"changed-pixel mask");
 std::vector<pa::Lab> outlineColors={pa::oklabFromSrgb({0,0,0}),pa::oklabFromSrgb({128,128,128}),pa::oklabFromSrgb({255,255,255})};labels.assign(side*side,2);std::vector<float> silhouette(side*side,0);for(int y=3;y<=5;y++)for(int x=3;x<=5;x++)silhouette[y*side+x]=1;pa::outlineLabels(labels,side,side,outlineColors,silhouette,.8);require(labels[40]==2&&labels[30]==1,"bounded inner outline affects boundary only");labels.assign(side*side,2);std::fill(silhouette.begin(),silhouette.end(),0.f);for(int y=2;y<7;y++)silhouette[y*side+4]=1;pa::outlineLabels(labels,side,side,outlineColors,silhouette,.8);for(int y=2;y<7;y++)require(labels[y*side+4]==2,"outline protects single-cell stroke");
 std::cout<<"GBA line coverage area="<<areaLines<<" feature="<<featureLines<<"; dark-dot expansions="<<expandedDots<<"; modern rules, counterexamples, mirror, masks and concurrency passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
