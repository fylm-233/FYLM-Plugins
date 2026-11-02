#include "../plugin/ae/FYLMParams.hpp"
#include <fstream>
#include <stdexcept>
int main(int argc,char** argv){if(argc!=2)return 1;std::ofstream out(argv[1]);if(!out)return 1;
 out<<"({name:'"<<fylm::Name<<"',match:'"<<fylm::Match<<"',rows:[";
 for(int i=1;i<fylm::Count;++i){const auto& p=fylm::Params[i-1];if(int(p.kind)>=4||std::strncmp(p.key,"library.",8)==0)continue;if(i>1)out<<',';
  out<<"{index:"<<i<<",key:'"<<p.key<<"',kind:"<<int(p.kind)<<",low:"<<p.low<<",high:"<<p.high<<",value:"<<p.value<<",choices:"<<p.choices<<"}";
 }out<<"]";
#ifdef FYLM_SHADERGLASS
 out<<",library:"<<fylm::sg::LibraryPanelJson;
#elif defined(FYLM_CRT_ONLY)
 out<<",recipes:"<<fylm::crt::VhsPanelJson;
#endif
 out<<"})\n";return out?0:1;
}
