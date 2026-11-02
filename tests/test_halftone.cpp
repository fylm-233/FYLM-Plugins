#include "dither/halftone.hpp"
#include <iostream>
#include <stdexcept>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
std::uint64_t hash(const std::vector<std::uint8_t>& a){std::uint64_t h=0xcbf29ce484222325ull;for(auto v:a){h^=v;h*=0x100000001b3ull;}return h;}
int main(){try{
 struct Case{int size,mode;std::uint64_t expected;};
 Case cases[]={{8,128,0x8530b123a7a4d966ull},{16,200,0x6aea2fbf03866a4eull},{96,128,0xaa68fe2ba62c37beull},
 {32,0,0x2e2e094e5fc1c495ull},{32,255,0xc0b014328067f325ull},{16,-1,0xb631719ac201152eull},{16,-2,0x0b49ffcfbcc77aa2ull}};
 for(auto c:cases){std::vector<std::uint8_t> a(c.size*c.size*4);
  for(int y=0;y<c.size;++y)for(int x=0;x<c.size;++x){int i=(y*c.size+x)*4;
   a[i]=c.mode>=0?c.mode:c.mode==-1?x*16:x*12+40;
   a[i+1]=c.mode>=0?c.mode:c.mode==-1?y*16:y*10+60;
   a[i+2]=c.mode>=0?c.mode:c.mode==-1?128:(x+y)*6;a[i+3]=255;
  }
  auto original=a,b=a;check(dither::applyHalftone(a,c.size,c.size).ok,"render failed");
  if(hash(a)!=c.expected){std::cerr<<"case "<<c.size<<'/'<<c.mode<<" got "<<std::hex<<hash(a)<<" expected "<<c.expected<<'\n';return 1;}
  check(dither::applyHalftone(b,c.size,c.size).ok&&a==b,"determinism");
  for(std::size_t i=3;i<a.size();i+=4)original[i]=static_cast<std::uint8_t>(i%256);
  b=original;check(dither::applyHalftone(b,c.size,c.size).ok,"alpha render");
  for(std::size_t i=3;i<b.size();i+=4)check(b[i]==original[i],"alpha changed");
 }
 std::vector<std::uint8_t> empty;check(dither::applyHalftone(empty,0,0).ok,"empty");
 std::vector<std::uint8_t> invalid(3);check(!dither::applyHalftone(invalid,1,1).ok,"invalid accepted");
 std::cout<<"7 halftone specification FNV vectors, determinism and alpha preservation passed.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
