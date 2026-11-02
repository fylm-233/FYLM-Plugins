#include "pixel_art/structure.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace pixel_art { namespace {
constexpr int offsets[8][2]={{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,1},{1,-1},{-1,1}};
void validate(const std::vector<unsigned short>& labels,int w,int h,const std::vector<Lab>& palette,const std::vector<float>& alpha){
 if(w<1||h<1||labels.size()!=std::size_t(w)*h||alpha.size()!=labels.size()||palette.empty()||palette.size()>256)throw std::invalid_argument("structure dimensions");
 for(auto i:labels)if(i>=palette.size())throw std::invalid_argument("structure palette label");
}
int neighbor(int index,int dx,int dy,int w,int h){int x=index%w+dx,y=index/w+dy;return x<0||y<0||x>=w||y>=h?-1:y*w+x;}
bool lineSupport(const std::vector<unsigned short>& labels,int index,int w,int h){
 for(int pair=0;pair<4;pair++){int a=neighbor(index,offsets[pair*2][0],offsets[pair*2][1],w,h),b=neighbor(index,offsets[pair*2+1][0],offsets[pair*2+1][1],w,h);if(a>=0&&b>=0&&labels[a]==labels[index]&&labels[b]==labels[index])return true;}return false;
}
bool protectedDetail(const std::vector<unsigned short>& labels,int index,int w,int h,const std::vector<Lab>& colors,const std::vector<float>& alpha,double protection){
 if(protection<=0)return false;if(lineSupport(labels,index,w,h))return true;
 // Keep endpoints with a collinear continuation; a perpendicular one-cell spur
 // attached to a long stroke does not satisfy this condition.
 for(auto& d:offsets){int a=neighbor(index,d[0],d[1],w,h),b=neighbor(index,d[0]*2,d[1]*2,w,h);if(a>=0&&b>=0&&labels[a]==labels[index]&&labels[b]==labels[index])return true;}
 auto center=colors[labels[index]];double maximum=0,peak=1;int valid=0;
 for(auto& d:offsets){int n=neighbor(index,d[0],d[1],w,h);if(n<0||alpha[n]<.01)continue;auto other=colors[labels[n]];maximum=std::max(maximum,std::sqrt(labDistanceSquared(center,other)));peak=std::min(peak,center.l-other.l);valid++;}
 return valid>0&&(maximum>.075+.15*(1-protection)||peak>.08);
}
}
std::vector<unsigned char> refineLabels(std::vector<unsigned short>& labels,int w,int h,const std::vector<Lab>& palette,const std::vector<float>& alpha,const Params& p,IProgress* progress){
 validate(labels,w,h,palette,alpha);const auto original=labels;const std::size_t count=labels.size();
 struct Region{std::size_t begin=0,size=0;unsigned short color=0;};
 for(int iteration=0;iteration<p.cleanupIterations;iteration++){
  if(progress&&progress->cancelled())throw std::runtime_error("pa.error.cancelled");
  const auto previous=labels;auto next=previous;
  if(p.mergeThreshold>0&&p.maxClusterArea>0){
   std::vector<int> regions(count,-1);std::vector<std::size_t> members;members.reserve(count);std::vector<Region> components;components.reserve(count);
   for(std::size_t seed=0;seed<count;seed++){
    if(seed%4096==0&&progress&&progress->cancelled())throw std::runtime_error("pa.error.cancelled");
    if(regions[seed]>=0||alpha[seed]<.01)continue;int id=int(components.size());std::size_t begin=members.size();members.push_back(seed);regions[seed]=id;
    for(std::size_t pos=begin;pos<members.size();pos++){auto index=members[pos];for(int k=0;k<4;k++){int n=neighbor(int(index),offsets[k][0],offsets[k][1],w,h);if(n>=0&&regions[n]<0&&alpha[n]>=.01&&previous[n]==previous[seed]){regions[n]=id;members.push_back(std::size_t(n));}}}
    components.push_back({begin,members.size()-begin,previous[seed]});
   }
   // Decisions read one immutable graph. Equal-size ties flow toward lower labels,
   // so two small regions cannot exchange colors on alternating iterations.
   for(std::size_t id=0;id<components.size();id++){const auto& region=components[id];if(region.size>std::size_t(p.maxClusterArea))continue;
    std::array<int,256> votes{};bool keep=false;
    for(std::size_t j=region.begin;j<region.begin+region.size;j++){auto index=members[j];if(protectedDetail(previous,int(index),w,h,palette,alpha,p.detailProtection)){keep=true;break;}for(int k=0;k<4;k++){int n=neighbor(int(index),offsets[k][0],offsets[k][1],w,h);if(n<0||regions[n]<0||regions[n]==int(id))continue;auto& other=components[regions[n]];if(other.size<region.size||(other.size==region.size&&other.color>=region.color))continue;votes[previous[n]]++;}}
    if(keep)continue;int best=-1;double error=1e100;int support=0;
    for(std::size_t color=0;color<palette.size();color++){if(!votes[color])continue;double e=std::sqrt(labDistanceSquared(palette[region.color],palette[color]));if(e>p.mergeThreshold)continue;if(votes[color]>support||(votes[color]==support&&e<error)){best=int(color);support=votes[color];error=e;}}
    if(best>=0)for(std::size_t j=region.begin;j<region.begin+region.size;j++)next[members[j]]=static_cast<unsigned short>(best);
   }
  }
  // Per-rule snapshots: local cleanup reads the cluster result once, then all rules
  // write a new buffer. The same outcomes hold for forward and reverse traversal.
  const auto local=next;
  for(std::size_t index=0;index<count;index++){
   if(index%4096==0&&progress&&progress->cancelled())throw std::runtime_error("pa.error.cancelled");
   if(alpha[index]<.01||protectedDetail(local,int(index),w,h,palette,alpha,p.detailProtection))continue;
   std::array<int,256> votes{};int same=0,cardinalSame=0,neighbors=0;for(int k=0;k<8;k++){auto& d=offsets[k];int n=neighbor(int(index),d[0],d[1],w,h);if(n<0||alpha[n]<.01)continue;votes[local[n]]++;neighbors++;if(local[n]==local[index]){same++;if(k<4)cardinalSame++;}}
   if(neighbors<5)continue;int candidate=0;for(std::size_t c=1;c<palette.size();c++)if(votes[c]>votes[candidate])candidate=int(c);
   if(candidate==local[index]||std::sqrt(labDistanceSquared(palette[local[index]],palette[candidate]))>.06)continue;
   bool speck=p.cleanSpeckles&&same==0&&votes[candidate]>=6;
   bool spur=p.cleanSpurs&&cardinalSame==1&&votes[candidate]>=5;
   // Only trim a short unsupported corner; through-lines and high-contrast
   // highlights were excluded above. This rule is separately disabled by default.
   bool stair=p.cleanStairs&&same==2&&!lineSupport(local,int(index),w,h)&&votes[candidate]>=6;
   if(speck||spur||stair)next[index]=static_cast<unsigned short>(candidate);
  }
  labels.swap(next);if(labels==previous)break;
 }
 std::vector<unsigned char> changed(count);for(std::size_t i=0;i<count;i++)changed[i]=labels[i]!=original[i];return changed;
}
void outlineLabels(std::vector<unsigned short>& labels,int w,int h,const std::vector<Lab>& palette,const std::vector<float>& alpha,double strength){
 validate(labels,w,h,palette,alpha);if(strength<=0)return;auto before=labels;
 for(std::size_t index=0;index<labels.size();index++){
  if(alpha[index]<.01)continue;int same=0;for(int k=0;k<4;k++){int n=neighbor(int(index),offsets[k][0],offsets[k][1],w,h);if(n>=0&&alpha[n]>=.01&&before[n]==before[index])same++;}bool alphaLine=false;for(int pair=0;pair<4;pair++){int a=neighbor(int(index),offsets[pair*2][0],offsets[pair*2][1],w,h),b=neighbor(int(index),offsets[pair*2+1][0],offsets[pair*2+1][1],w,h);if(a>=0&&b>=0&&alpha[a]>=.01&&alpha[b]>=.01&&before[a]==before[index]&&before[b]==before[index])alphaLine=true;}if(same<=2&&alphaLine)continue;auto c=palette[before[index]];bool boundary=false;int covered=0;
  for(int k=0;k<4;k++){int n=neighbor(int(index),offsets[k][0],offsets[k][1],w,h);if(n<0||alpha[n]<.01)boundary=true;else{covered++;if(palette[before[n]].l+.18<c.l)boundary=true;}}
  if(!boundary||covered<2)continue;Lab target{std::max(0.,c.l-.3*strength),c.a,c.b};auto best=before[index];double error=labDistanceSquared(c,target);
  for(std::size_t j=0;j<palette.size();j++){auto p=palette[j];if(p.l>=c.l||std::abs(p.a-c.a)+std::abs(p.b-c.b)>.18)continue;double d=labDistanceSquared(p,target);if(d<error){error=d;best=static_cast<unsigned short>(j);}}
  labels[index]=best;
 }
}
}
