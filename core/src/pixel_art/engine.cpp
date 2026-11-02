#include "pixel_art/core.hpp"
#include "pixel_art/structure.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <stdexcept>
namespace pixel_art { namespace {
struct Failure{ErrorCode code;};
[[noreturn]] void fail(ErrorCode e){throw Failure{e};}
int pixelSize(PixelFormat f){switch(f){case PixelFormat::RGBA8:case PixelFormat::ARGB8Premultiplied:return 4;case PixelFormat::RGBA16:case PixelFormat::ARGB16_32768Premultiplied:return 8;case PixelFormat::RGBA32F:case PixelFormat::ARGB32FPremultiplied:return 16;}fail(ErrorCode::UnsupportedFormat);}
double channel(const unsigned char* p,PixelFormat f,int c){
 double v=0;if(f==PixelFormat::RGBA8)v=p[c]/255.;else if(f==PixelFormat::RGBA16){std::uint16_t n;std::memcpy(&n,p+c*2,2);v=n/65535.;}else{float n;std::memcpy(&n,p+c*4,4);v=n;}
 return std::isfinite(v)?std::clamp(v,0.,1.):0.; // Explicit SDR mapping before quantization.
}
void store(unsigned char* p,PixelFormat f,int c,double v){v=std::clamp(v,0.,1.);if(f==PixelFormat::RGBA8)p[c]=toU8(v*255);else if(f==PixelFormat::RGBA16){std::uint16_t n=std::uint16_t(std::lround(v*65535));std::memcpy(p+c*2,&n,2);}else{float n=float(v);std::memcpy(p+c*4,&n,4);}}
void storePixel(unsigned char* pixel,PixelFormat format,const double* rgb,double alpha){
 if(format==PixelFormat::ARGB8Premultiplied||format==PixelFormat::ARGB16_32768Premultiplied||format==PixelFormat::ARGB32FPremultiplied){
  // Match the existing host's float32 intermediate and float multiplication.
  float a=float(std::clamp(alpha,0.,1.)),values[4]={a,float(std::clamp(rgb[0],0.,1.))*a,float(std::clamp(rgb[1],0.,1.))*a,float(std::clamp(rgb[2],0.,1.))*a};
  for(int i=0;i<4;i++)if(format==PixelFormat::ARGB8Premultiplied)pixel[i]=toU8(values[i]*255.f);else if(format==PixelFormat::ARGB16_32768Premultiplied){auto n=std::uint16_t(std::lround(values[i]*32768.f));std::memcpy(pixel+i*2,&n,2);}else std::memcpy(pixel+i*4,&values[i],4);
 }else{for(int k=0;k<3;k++)store(pixel,format,k,alpha==0?0:rgb[k]);store(pixel,format,3,alpha);}
}
void checkView(ConstImageView v){int size=pixelSize(v.format);if(!v.data||v.width<1||v.height<1||v.width>32768||v.height>32768)fail(ErrorCode::InvalidImage);
 if(v.rowBytes<std::size_t(v.width)*size||std::size_t(v.height)>SIZE_MAX/v.rowBytes)fail(ErrorCode::InvalidStride);
 if(!std::isfinite(v.originX)||!std::isfinite(v.originY)||std::abs(v.originX)>1e9||std::abs(v.originY)>1e9)fail(ErrorCode::Overflow);
}
void tick(IProgress* p,double value){if(p){if(p->cancelled())fail(ErrorCode::Cancelled);p->report(value);}}
struct Cell{double r=0,g=0,b=0,alpha=0;};
struct Color{Cell rgb;Lab lab;};
std::vector<Color> working(const std::vector<Rgb8>& palette){std::vector<Color> out;for(auto p:palette){Cell c{srgbToLinear(p.r/255.),srgbToLinear(p.g/255.),srgbToLinear(p.b/255.),1};out.push_back({c,oklabFromLinear(c.r,c.g,c.b)});}return out;}
std::size_t nearest(const std::vector<Color>& p,Lab c){std::size_t best=0;double d=1e100;for(std::size_t i=0;i<p.size();++i){double e=labDistanceSquared(p[i].lab,c);if(e<d){d=e;best=i;}}return best;}
int mod(long long v,int n){int r=int(v%n);return r<0?r+n:r;}
double bayer(long long x,long long y,int n){int rank=0;for(int bit=1;bit<n;bit*=2){int a=(mod(x,n)&bit)?1:0,b=(mod(y,n)&bit)?1:0;rank=rank*4+((a^b)*2+b);}return (rank+.5)/(n*n);}
long long cellIndex(double value,double anchor,double step){double f=std::floor((value-anchor)/step);if(!std::isfinite(f)||std::abs(f)>1e12)fail(ErrorCode::Overflow);return static_cast<long long>(f);}
}
std::vector<Rgb8> pc98Palette(){
 // Original FYLM illustration palette; visual PC-98 preset, not hardware emulation.
 return {{20,16,32},{50,32,61},{78,48,78},{118,68,98},{166,103,119},{211,151,151},{244,201,172},{255,235,204},
         {34,50,78},{48,80,123},{71,127,155},{112,180,181},{43,83,64},{87,127,80},{158,164,102},{225,207,142}};
}
std::vector<Rgb8> stylePalette(Style style){
 if(style==Style::Basic)return pc98Palette();
 const Rgb8 bases[]={{211,128,100},{217,173,93},{117,162,91},{74,164,158},{87,123,205},{169,101,177},{145,148,166},{206,155,138}};
 int hues=style==Style::GBA?6:8,steps=style==Style::GBA?8:12;std::vector<Rgb8> out;out.reserve(hues*steps);
 for(int h=0;h<hues;h++)for(int j=0;j<steps;j++){double t=(j+.6)/(steps+.2);auto tone=[&](int c){return toU8(t<.55?c*t/.55:c+(255-c)*(t-.55)/.45*.85);};out.push_back({tone(bases[h].r),tone(bases[h].g),tone(bases[h].b)});}
 out.front()={20,20,28};out.back()={244,241,230};return out;
}
std::vector<Rgb8> extractPalette(ConstImageView src,ColorEncoding encoding,int colors,IProgress* progress){
 checkView(src);if(src.format!=PixelFormat::RGBA8&&src.format!=PixelFormat::RGBA16&&src.format!=PixelFormat::RGBA32F)fail(ErrorCode::UnsupportedFormat);if(colors<2||colors>256)fail(ErrorCode::InvalidParams);
 struct Bin{double r=0,g=0,b=0,w=0;};std::vector<Bin> hist(32768);int ps=pixelSize(src.format);
 // Fixed spatial stride bounds work while remaining independent of frame/render order.
 int stride=std::max(1,int(std::sqrt(double(src.width)*src.height/131072.)));
 for(int y=0;y<src.height;y+=stride){if(y%32==0)tick(progress,0);auto row=static_cast<const unsigned char*>(src.data)+std::size_t(y)*src.rowBytes;
  for(int x=0;x<src.width;x+=stride){auto p=row+std::size_t(x)*ps;double a=channel(p,src.format,3);if(a==0)continue;double c[3];for(int j=0;j<3;j++){c[j]=channel(p,src.format,j);if(encoding==ColorEncoding::LinearSrgb)c[j]=linearToSrgb(c[j]);}
   int index=(int(c[0]*31)<<10)|(int(c[1]*31)<<5)|int(c[2]*31);auto& b=hist[index];b.r+=c[0]*a;b.g+=c[1]*a;b.b+=c[2]*a;b.w+=a;}}
 std::vector<int> occupied;for(int i=0;i<32768;i++)if(hist[i].w>0)occupied.push_back(i);if(occupied.empty())return {};
 std::vector<std::vector<int>> boxes{occupied};
 while(int(boxes.size())<colors){std::size_t which=boxes.size();double score=-1;int axis=0;
  for(std::size_t i=0;i<boxes.size();i++){if(boxes[i].size()<2)continue;double lo[3]={1,1,1},hi[3]={0,0,0},weight=0;for(int n:boxes[i]){auto& b=hist[n];double c[3]={b.r/b.w,b.g/b.w,b.b/b.w};weight+=b.w;for(int j=0;j<3;j++){lo[j]=std::min(lo[j],c[j]);hi[j]=std::max(hi[j],c[j]);}}
   for(int j=0;j<3;j++){double s=(hi[j]-lo[j])*weight;if(s>score){score=s;which=i;axis=j;}}}
  if(which==boxes.size())break;auto box=std::move(boxes[which]);auto value=[&](int n){auto& b=hist[n];return (axis==0?b.r:axis==1?b.g:b.b)/b.w;};
  std::stable_sort(box.begin(),box.end(),[&](int a,int b){double x=value(a),y=value(b);return x==y?a<b:x<y;});double total=0;for(int n:box)total+=hist[n].w;double acc=0;std::size_t split=1;for(;split<box.size();split++){acc+=hist[box[split-1]].w;if(acc>=total*.5)break;}split=std::min(split,box.size()-1);
  boxes[which]={box.begin(),box.begin()+split};boxes.push_back({box.begin()+split,box.end()});tick(progress,0);}
 std::vector<Rgb8> out;for(auto& box:boxes){Bin sum;for(int i:box){auto b=hist[i];sum.r+=b.r;sum.g+=b.g;sum.b+=b.b;sum.w+=b.w;}Rgb8 c{toU8(sum.r/sum.w*255),toU8(sum.g/sum.w*255),toU8(sum.b/sum.w*255)};out.push_back(c);}
 std::stable_sort(out.begin(),out.end(),[](auto a,auto b){auto x=oklabFromSrgb(a),y=oklabFromSrgb(b);if(x.l!=y.l)return x.l<y.l;if(a.r!=b.r)return a.r<b.r;if(a.g!=b.g)return a.g<b.g;return a.b<b.b;});out.erase(std::unique(out.begin(),out.end(),[](auto a,auto b){return a.r==b.r&&a.g==b.g&&a.b==b.b;}),out.end());return out;
}
Result renderSerial(ConstImageView src,ImageView dst,const Params& original,IProgress* progress)noexcept{try{
 checkView(src);checkView({dst.data,dst.width,dst.height,dst.rowBytes,dst.format,dst.originX,dst.originY});
 if(src.format!=PixelFormat::RGBA8&&src.format!=PixelFormat::RGBA16&&src.format!=PixelFormat::RGBA32F)fail(ErrorCode::UnsupportedFormat);
 int specification=int(original.pc98ColorSpec);if(specification<0||specification>5||original.palette.size()>256)fail(ErrorCode::InvalidParams);
 bool direct=original.pc98ColorSpec==PC98ColorSpec::RGB565||original.pc98ColorSpec==PC98ColorSpec::RGB888;
 Params prepared;const Params* active=&original;
 if(specification){if(!original.pc98||original.style!=Style::Basic||original.outlineStrength!=0||(original.pc98PaletteBits!=6&&original.pc98PaletteBits!=8))fail(ErrorCode::InvalidParams);
  prepared=original;prepared.palette=direct?std::vector<Rgb8>{{0,0,0}}:original.pc98ColorSpec==PC98ColorSpec::Digital8||original.palette.empty()?pc98HardwarePalette(original.pc98ColorSpec,256,original.pc98PaletteBits):pc98ConformPalette(original.palette,original.pc98ColorSpec,original.pc98PaletteBits);active=&prepared;
 }const Params& p=*active;
 if(p.palette.empty())fail(ErrorCode::EmptyPalette);if(p.palette.size()>256||p.grid.block<1||p.grid.block>64||!std::isfinite(p.grid.pixelAspect)||p.grid.pixelAspect<.01||p.grid.pixelAspect>100)fail(ErrorCode::InvalidParams);
 if(!std::isfinite(p.alphaThreshold)||p.alphaThreshold<0||p.alphaThreshold>1||!std::isfinite(p.ditherStrength)||p.ditherStrength<0||p.ditherStrength>1||!std::isfinite(p.edgeProtection)||p.edgeProtection<0||p.edgeProtection>1)fail(ErrorCode::InvalidParams);
 if(p.encoding!=ColorEncoding::Srgb&&p.encoding!=ColorEncoding::LinearSrgb)fail(ErrorCode::InvalidParams);
 if(p.grid.sampling!=Sampling::Area&&p.grid.sampling!=Sampling::Nearest&&p.grid.sampling!=Sampling::Feature)fail(ErrorCode::InvalidParams);
 if(p.style!=Style::Basic&&p.style!=Style::GBA&&p.style!=Style::Modern)fail(ErrorCode::InvalidParams);
 if(!std::isfinite(p.featureStrength)||p.featureStrength<0||p.featureStrength>1||!std::isfinite(p.outlineStrength)||p.outlineStrength<0||p.outlineStrength>1||!std::isfinite(p.mergeThreshold)||p.mergeThreshold<0||p.mergeThreshold>.2||!std::isfinite(p.detailProtection)||p.detailProtection<0||p.detailProtection>1||p.maxClusterArea<0||p.maxClusterArea>32||p.cleanupIterations<1||p.cleanupIterations>3)fail(ErrorCode::InvalidParams);
 double dx=p.grid.downsampleX,dy=p.grid.downsampleY;if(!std::isfinite(dx)||!std::isfinite(dy)||dx<1./32768||dy<1./32768||dx>1||dy>1)fail(ErrorCode::InvalidParams);
 if(p.ditherMatrix!=0&&p.ditherMatrix!=2&&p.ditherMatrix!=4&&p.ditherMatrix!=8)fail(ErrorCode::InvalidParams);
 const double bx=p.grid.block,by=p.grid.block*p.grid.pixelAspect,sx=1/dx,sy=1/dy;
 // Include one cell halo for gradient protection. Source dependency is full input.
 long long x0=cellIndex(src.originX,p.grid.originX,bx)-1,y0=cellIndex(src.originY,p.grid.originY,by)-1;
 long long x1=cellIndex(src.originX+src.width*sx-1e-6,p.grid.originX,bx)+1,y1=cellIndex(src.originY+src.height*sy-1e-6,p.grid.originY,by)+1;
 auto cols=std::uint64_t(x1-x0+1),rows=std::uint64_t(y1-y0+1);std::size_t perCell=p.style==Style::Modern?128:p.outlineStrength>0?48:sizeof(Cell)+sizeof(unsigned short)+sizeof(float);
 auto sourceAddress=reinterpret_cast<std::uintptr_t>(src.data),destinationAddress=reinterpret_cast<std::uintptr_t>(dst.data);
 const bool overlapping=sourceAddress<=destinationAddress?destinationAddress-sourceAddress<src.rowBytes*src.height:sourceAddress-destinationAddress<dst.rowBytes*dst.height;
 const bool streaming=p.style==Style::Basic&&p.outlineStrength==0&&!overlapping;
 const bool ordered=p.pc98&&p.pc98ColorSpec!=PC98ColorSpec::RGB888;
 const bool cachedLab=streaming&&ordered&&p.edgeProtection>0&&p.ditherMatrix!=0&&p.ditherStrength>0;
 constexpr std::size_t cacheBudget=512*1024;
 if(!cols||!rows||cols>SIZE_MAX/rows/perCell)fail(ErrorCode::OutOfMemory);
 auto workspace=streaming?cols*(3*sizeof(Cell)+(cachedLab?3*sizeof(Lab):0)+sizeof(unsigned short)+1+(direct?sizeof(std::uint32_t):0))+std::size_t(dst.width)*sizeof(long long)*2:cols*rows*perCell;
 if(workspace>p.memoryBudget||cacheBudget>p.memoryBudget-workspace)fail(ErrorCode::OutOfMemory);
 std::vector<Cell> cells(std::size_t(cols*(streaming?3:rows)));auto palette=working(p.palette);int sp=pixelSize(src.format),dp=pixelSize(dst.format);
 std::vector<Lab> rowLabs(cachedLab?std::size_t(cols*3):0);
 struct Transfer{double key=-1,value=0;};std::array<Transfer,4096> transfer{};
 auto linearize=[&](double value){auto& entry=transfer[std::size_t(value*65535)&4095];if(entry.key!=value){entry.key=value;entry.value=srgbToLinear(value);}return entry.value;};
 auto sample=[&](long long gx,long long gy){Cell c;double left=p.grid.originX+gx*bx,top=p.grid.originY+gy*by;
  int a=std::max(0,int(std::floor((left-src.originX)/sx))),b=std::min(src.width,int(std::ceil((left+bx-src.originX)/sx)));
  int u=std::max(0,int(std::floor((top-src.originY)/sy))),v=std::min(src.height,int(std::ceil((top+by-src.originY)/sy)));
  if(a>=b||u>=v)return c;double area=0;
  if(p.grid.sampling==Sampling::Nearest){int xx=std::clamp(int((left+bx*.5-src.originX)/sx),0,src.width-1),yy=std::clamp(int((top+by*.5-src.originY)/sy),0,src.height-1);a=xx;b=xx+1;u=yy;v=yy+1;}
  for(int y=u;y<v;y++){auto row=static_cast<const unsigned char*>(src.data)+std::size_t(y)*src.rowBytes;for(int x=a;x<b;x++){double weight=1;
   if(p.grid.sampling!=Sampling::Nearest)weight=std::max(0.,std::min(left+bx,src.originX+(x+1)*sx)-std::max(left,src.originX+x*sx))*std::max(0.,std::min(top+by,src.originY+(y+1)*sy)-std::max(top,src.originY+y*sy));
   auto pixel=row+std::size_t(x)*sp;double alpha=channel(pixel,src.format,3);double rgb[3];for(int k=0;k<3;k++){rgb[k]=channel(pixel,src.format,k);if(p.encoding==ColorEncoding::Srgb)rgb[k]=linearize(rgb[k]);}c.r+=rgb[0]*alpha*weight;c.g+=rgb[1]*alpha*weight;c.b+=rgb[2]*alpha*weight;c.alpha+=alpha*weight;area+=weight;}}
  if(c.alpha>0){c.r/=c.alpha;c.g/=c.alpha;c.b/=c.alpha;}if(area>0)c.alpha/=area;
  if(p.grid.sampling==Sampling::Feature&&p.featureStrength>0&&c.alpha>0){
   struct Group{int key=0,count=0;double r=0,g=0,b=0,weight=0;};std::array<Group,49> groups{};std::array<int,49> tags;tags.fill(-1);int used=0;double total=0;
   std::array<int,7> sampleX{},sampleY{};std::array<double,7> weightX{},weightY{};
   for(int i=0;i<7;i++){sampleX[i]=int(std::floor((left+(i+.5)*bx/7-src.originX)/sx));sampleY[i]=int(std::floor((top+(i+.5)*by/7-src.originY)/sy));}
   for(int i=0;i<7;i++){weightX[i]=1./std::count(sampleX.begin(),sampleX.end(),sampleX[i]);weightY[i]=1./std::count(sampleY.begin(),sampleY.end(),sampleY[i]);}
   // Bounded 7x7 samples. Connected minor color groups may represent lines; an
   // isolated dark sample cannot win merely by being dark.
   for(int yy=0;yy<7;yy++)for(int xx=0;xx<7;xx++){int ix=sampleX[xx],iy=sampleY[yy];if(ix<0||iy<0||ix>=src.width||iy>=src.height)continue;auto pixel=static_cast<const unsigned char*>(src.data)+std::size_t(iy)*src.rowBytes+std::size_t(ix)*sp;double alphaWeight=channel(pixel,src.format,3)*weightX[xx]*weightY[yy];if(alphaWeight<=0)continue;double rgb[3],encoded[3];for(int k=0;k<3;k++){auto channelValue=channel(pixel,src.format,k);rgb[k]=p.encoding==ColorEncoding::Srgb?linearize(channelValue):channelValue;encoded[k]=p.encoding==ColorEncoding::Srgb?channelValue:linearToSrgb(channelValue);}
    // Canonicalize only the coarse classification key. AE's 16-bit conversion
    // can straddle exact n/15 boundaries; retain full precision for group means.
    auto bin=[](double value){return int(toU8(value*255))*15/255;};
    int key=(bin(encoded[0])<<8)|(bin(encoded[1])<<4)|bin(encoded[2]),g=0;while(g<used&&groups[g].key!=key)g++;if(g==used){groups[g].key=key;used++;}auto& group=groups[g];group.count++;group.weight+=alphaWeight;group.r+=rgb[0]*alphaWeight;group.g+=rgb[1]*alphaWeight;group.b+=rgb[2]*alphaWeight;tags[yy*7+xx]=g;total+=alphaWeight;
   }
   auto mean=oklabFromLinear(c.r,c.g,c.b);int chosen=-1;double best=0;
   for(int g=0;g<used;g++){auto& group=groups[g];if(group.weight<=0)continue;double coverage=group.weight/total;auto lab=oklabFromLinear(group.r/group.weight,group.g/group.weight,group.b/group.weight);double contrast=std::sqrt(labDistanceSquared(lab,mean));
    if(coverage>.55&&p.featureStrength>=.35){double score=coverage*.05;if(score>best){best=score;chosen=g;}}
    if(coverage<.075||coverage>.4||contrast<.07+.18*(1-p.featureStrength))continue;
    std::array<bool,49> visited{};int largest=0;bool spans=false;
    for(int seed=0;seed<49;seed++){if(tags[seed]!=g||visited[seed])continue;std::array<int,49> queue{};int head=0,tail=1;queue[0]=seed;visited[seed]=true;int minX=seed%7,maxX=minX,minY=seed/7,maxY=minY;
     while(head<tail){int q=queue[head++],qx=q%7,qy=q/7;minX=std::min(minX,qx);maxX=std::max(maxX,qx);minY=std::min(minY,qy);maxY=std::max(maxY,qy);for(int oy=-1;oy<=1;oy++)for(int ox=-1;ox<=1;ox++){int nx=qx+ox,ny=qy+oy,n=ny*7+nx;if(nx<0||ny<0||nx>=7||ny>=7||visited[n]||tags[n]!=g)continue;visited[n]=true;queue[tail++]=n;}}
     largest=std::max(largest,tail);if(tail>=4&&(maxX-minX>=4||maxY-minY>=4))spans=true;
    }
    if(spans&&largest>=group.count*.6){double score=contrast*p.featureStrength;if(score>best){best=score;chosen=g;}}
   }
   if(chosen>=0){auto group=groups[chosen];c.r=group.r/group.weight;c.g=group.g/group.weight;c.b=group.b/group.weight;}
  }
  return c;};
 struct Decision{double r=0,g=0,b=0,strength=0,amount=0;unsigned short a=0,bIndex=0;bool valid=false;};
 std::vector<Decision> decisions(p.pc98&&!direct?4096:0);
 auto choose=[&](Cell c,long long gx,long long gy,double edge,const Lab* knownLab=nullptr){
  double strength=p.ditherStrength*std::clamp(1-p.edgeProtection*edge*5,0.,1.);
  if(!p.pc98||p.ditherMatrix==0||strength<=0||c.alpha==0)return static_cast<unsigned short>(nearest(palette,knownLab?*knownLab:oklabFromLinear(c.r,c.g,c.b)));
  auto hash=(std::uint32_t(c.r*65535)*73856093u)^(std::uint32_t(c.g*65535)*19349663u)^(std::uint32_t(c.b*65535)*83492791u)^(std::uint32_t(strength*65535)*2654435761u);
  auto& cached=decisions[hash&4095];
  if(cached.valid&&cached.r==c.r&&cached.g==c.g&&cached.b==c.b&&cached.strength==strength)return bayer(gx,gy,p.ditherMatrix)<cached.amount?cached.bIndex:cached.a;
  auto lab=knownLab?*knownLab:oklabFromLinear(c.r,c.g,c.b);std::array<double,256> distance{};std::array<std::size_t,6> candidates{};std::size_t count=0;
  for(std::size_t i=0;i<palette.size();i++){distance[i]=labDistanceSquared(lab,palette[i].lab);std::size_t pos=0;while(pos<count&&distance[candidates[pos]]<=distance[i])pos++;if(pos<6){for(std::size_t j=std::min(count,std::size_t(5));j>pos;j--)candidates[j]=candidates[j-1];candidates[pos]=i;count=std::min(count+1,std::size_t(6));}}
  auto best=candidates[0];double error=distance[best]*strength,amount=0;std::size_t ca=best,cb=best;
  for(std::size_t i=0;i<count;i++)for(std::size_t j=i+1;j<count;j++){auto ia=candidates[i],ib=candidates[j];auto a=palette[ia].rgb,b=palette[ib].rgb;double r=b.r-a.r,g=b.g-a.g,blue=b.b-a.b,den=r*r+g*g+blue*blue;if(den<1e-12)continue;double t=std::clamp(((c.r-a.r)*r+(c.g-a.g)*g+(c.b-a.b)*blue)/den,0.,1.);
   double score=labDistanceSquared(lab,oklabFromLinear(a.r+t*r,a.g+t*g,a.b+t*blue))+.015*(1-strength)*labDistanceSquared(palette[ia].lab,palette[ib].lab)+.0003;
   if(score<error){error=score;amount=t;ca=ia;cb=ib;}}
  cached={c.r,c.g,c.b,strength,amount,static_cast<unsigned short>(ca),static_cast<unsigned short>(cb),true};
  return static_cast<unsigned short>(bayer(gx,gy,p.ditherMatrix)<amount?cb:ca);
 };
 auto edgeAt=[&](std::uint64_t x,std::uint64_t y){double edge=0;if(!ordered||p.edgeProtection==0||p.ditherMatrix==0||p.ditherStrength==0)return edge;auto at=[&](std::uint64_t xx,std::uint64_t yy){return cells[std::size_t((streaming?yy%3:yy)*cols+xx)];};auto c=at(x,y);auto lab=cachedLab?rowLabs[std::size_t((y%3)*cols+x)]:oklabFromLinear(c.r,c.g,c.b);
  for(auto delta:std::array<std::pair<int,int>,4>{{{-1,0},{1,0},{0,-1},{0,1}}}){long long xx=static_cast<long long>(x)+delta.first,yy=static_cast<long long>(y)+delta.second;if(xx<0||yy<0||xx>=static_cast<long long>(cols)||yy>=static_cast<long long>(rows))continue;auto n=at(xx,yy);auto neighborLab=cachedLab?rowLabs[std::size_t((yy%3)*cols+xx)]:oklabFromLinear(n.r,n.g,n.b);edge=std::max(edge,std::sqrt(labDistanceSquared(lab,neighborLab)));edge=std::max(edge,std::abs(n.alpha-c.alpha));}return edge;
 };
 auto directCode=[&](Cell c,long long gx,long long gy,double edge){double noise=p.pc98ColorSpec==PC98ColorSpec::RGB565&&p.ditherMatrix?(bayer(gx,gy,p.ditherMatrix)-.5)*p.ditherStrength*std::clamp(1-p.edgeProtection*edge*5,0.,1.):0;auto color=pc98DirectColor(linearToSrgb(c.r),linearToSrgb(c.g),linearToSrgb(c.b),p.pc98ColorSpec,noise);return (std::uint32_t(color.r)<<16)|(std::uint32_t(color.g)<<8)|color.b;};
 auto unpack=[&](std::uint32_t code){std::array<double,3> rgb={double((code>>16)&255)/255.,double((code>>8)&255)/255.,double(code&255)/255.};if(p.encoding==ColorEncoding::LinearSrgb)for(auto& c:rgb)c=srgbToLinear(c);return rgb;};
 if(streaming){
  std::vector<unsigned short> labels(std::size_t(cols),0);std::vector<long long> column(dst.width),activeColumns;activeColumns.reserve(dst.width);std::vector<unsigned char> needed(std::size_t(cols),0);
  std::vector<std::uint32_t> directCodes(direct?std::size_t(cols):0);
  for(int x=0;x<dst.width;x++){auto gx=column[x]=cellIndex(dst.originX+(x+.5)*sx,p.grid.originX,bx)-x0;if(gx<0||gx>=static_cast<long long>(cols))continue;if(activeColumns.empty()||activeColumns.back()!=gx)activeColumns.push_back(gx);needed[std::size_t(gx)]=1;if(gx>0)needed[std::size_t(gx-1)]=1;if(gx+1<static_cast<long long>(cols))needed[std::size_t(gx+1)]=1;}
  for(int y=0;y<dst.height;y++){if(y%16==0)tick(progress,0);std::memset(static_cast<unsigned char*>(dst.data)+std::size_t(y)*dst.rowBytes,0,std::size_t(dst.width)*dp);}
  std::array<std::uint64_t,3> cachedRows{UINT64_MAX,UINT64_MAX,UINT64_MAX};
  auto fill=[&](std::uint64_t y){if(cachedRows[y%3]==y)return;for(std::uint64_t x=0;x<cols;x++){if(x%4096==0&&progress&&progress->cancelled())fail(ErrorCode::Cancelled);if(needed[std::size_t(x)]){auto index=std::size_t((y%3)*cols+x);auto c=cells[index]=sample(x0+static_cast<long long>(x),y0+static_cast<long long>(y));if(cachedLab)rowLabs[index]=oklabFromLinear(c.r,c.g,c.b);}}cachedRows[y%3]=y;};
  int outputY=0;
  std::vector<std::array<double,3>> outputPalette;for(auto color:p.palette){std::array<double,3> value={color.r/255.,color.g/255.,color.b/255.};if(p.encoding==ColorEncoding::LinearSrgb)for(auto& channelValue:value)channelValue=srgbToLinear(channelValue);outputPalette.push_back(value);}
  while(outputY<dst.height){
   tick(progress,double(outputY)/dst.height);auto logicalY=cellIndex(dst.originY+(outputY+.5)*sy,p.grid.originY,by)-y0;
   if(logicalY<0||logicalY>=static_cast<long long>(rows)){outputY++;continue;}auto y=static_cast<std::uint64_t>(logicalY);
   if(y>0)fill(y-1);fill(y);if(y+1<rows)fill(y+1);
   std::size_t processed=0;for(auto x:activeColumns){if(processed++%4096==0&&progress&&progress->cancelled())fail(ErrorCode::Cancelled);auto index=std::size_t((y%3)*cols+x);if(direct)directCodes[std::size_t(x)]=directCode(cells[index],x0+x,y0+static_cast<long long>(y),edgeAt(x,y));else labels[std::size_t(x)]=choose(cells[index],x0+x,y0+static_cast<long long>(y),edgeAt(x,y),cachedLab?&rowLabs[index]:nullptr);}
   while(outputY<dst.height){auto gy=cellIndex(dst.originY+(outputY+.5)*sy,p.grid.originY,by)-y0;if(gy>static_cast<long long>(y))break;
    if(gy==static_cast<long long>(y)){auto row=static_cast<unsigned char*>(dst.data)+std::size_t(outputY)*dst.rowBytes;for(int x=0;x<dst.width;x++){auto gx=column[x];if(gx<0||gx>=static_cast<long long>(cols))continue;double alpha=cells[std::size_t((y%3)*cols+gx)].alpha;if(p.binaryAlpha)alpha=alpha>0&&alpha>=p.alphaThreshold?1:0;auto color=direct?unpack(directCodes[std::size_t(gx)]):outputPalette[labels[std::size_t(gx)]];auto pixel=row+std::size_t(x)*dp;storePixel(pixel,dst.format,color.data(),alpha);}}
    outputY++;
   }
  }
  tick(progress,1);return {true,ErrorCode::None,{}};
 }
 for(std::uint64_t y=0;y<rows;y++){tick(progress,.35*double(y)/rows);for(std::uint64_t x=0;x<cols;x++)cells[std::size_t(y*cols+x)]=sample(x0+static_cast<long long>(x),y0+static_cast<long long>(y));}
 std::array<std::array<double,3>,256> expandedPalette{};for(std::size_t i=0;i<p.palette.size();i++){auto c=p.palette[i];expandedPalette[i]={c.r/255.,c.g/255.,c.b/255.};if(p.encoding==ColorEncoding::LinearSrgb)for(auto& value:expandedPalette[i])value=srgbToLinear(value);}
 // Structure effects retain the full label graph; basic/PC-98 use bounded rows.
 std::vector<unsigned short> indices(cells.size());
 std::vector<std::uint32_t> directCodes(direct?cells.size():0);
 for(std::uint64_t y=0;y<rows;y++){tick(progress,.35+.45*double(y)/rows);for(std::uint64_t x=0;x<cols;x++){auto index=std::size_t(y*cols+x);if(direct)directCodes[index]=directCode(cells[index],x0+static_cast<long long>(x),y0+static_cast<long long>(y),edgeAt(x,y));else indices[index]=choose(cells[index],x0+static_cast<long long>(x),y0+static_cast<long long>(y),edgeAt(x,y));}}
 std::vector<unsigned short> unmodified;std::vector<unsigned char> changed;
 if(p.style==Style::Modern||p.outlineStrength>0){std::vector<float> alpha(cells.size());std::vector<Lab> labs;for(auto& c:palette)labs.push_back(c.lab);for(std::size_t i=0;i<cells.size();i++)alpha[i]=float(cells[i].alpha);
  if(p.style==Style::Modern&&p.debugChanges)unmodified=indices;
  if(p.style==Style::Modern){try{refineLabels(indices,int(cols),int(rows),labs,alpha,p,progress);}catch(...){if(progress&&progress->cancelled())fail(ErrorCode::Cancelled);throw;}}
  outlineLabels(indices,int(cols),int(rows),labs,alpha,p.outlineStrength);
  if(!unmodified.empty()){changed.resize(indices.size());for(std::size_t i=0;i<indices.size();i++)changed[i]=indices[i]!=unmodified[i];}
 }
 for(int y=0;y<dst.height;y++){if(y%16==0)tick(progress,.8+.2*double(y)/dst.height);auto row=static_cast<unsigned char*>(dst.data)+std::size_t(y)*dst.rowBytes;
  long long gy=cellIndex(dst.originY+(y+.5)*sy,p.grid.originY,by)-y0;for(int x=0;x<dst.width;x++){auto pixel=row+std::size_t(x)*dp;long long gx=cellIndex(dst.originX+(x+.5)*sx,p.grid.originX,bx)-x0;Cell c;std::size_t ix=0;bool valid=gx>=0&&gy>=0&&gx<static_cast<long long>(cols)&&gy<static_cast<long long>(rows);if(valid){ix=std::size_t(gy*cols+gx);c=cells[ix];}
   double alpha=c.alpha;if(p.style==Style::Modern&&p.debugChanges)alpha=valid&&!changed.empty()&&changed[ix]?alpha:0;
   if(p.binaryAlpha)alpha=alpha>0&&alpha>=p.alphaThreshold?1:0;auto color=valid?p.palette[indices[ix]]:Rgb8{};if(p.style==Style::Modern&&p.debugChanges)color={255,255,255};std::array<double,3> values=valid?expandedPalette[indices[ix]]:std::array<double,3>{};if(p.style==Style::Modern&&p.debugChanges)values={1,1,1};if(direct&&valid){auto rgb=unpack(directCodes[ix]);std::copy(rgb.begin(),rgb.end(),values.begin());}storePixel(pixel,dst.format,values.data(),alpha);
 }}tick(progress,1);return {true,ErrorCode::None,{}};
 }catch(const Failure& e){return {false,e.code,errorKey(e.code)};}catch(const std::bad_alloc&){return {false,ErrorCode::OutOfMemory,"pa.error.memory"};}catch(...){return {false,ErrorCode::Internal,"pa.error.internal"};}}
}
