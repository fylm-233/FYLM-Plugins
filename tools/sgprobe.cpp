#include "preset.hpp"
#include "json.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
using nlohmann::json;
int wmain(int argc,wchar_t** argv){json report;try{
 if(argc<8||argc>10){std::cerr<<"Usage: FYLM-sgprobe preset.fylmsg width height virtual_height frame output.rgba report.json [input.rgba8] [strength_0_to_1]\n";return 2;}
 bool validation=argc==9&&std::wstring(argv[8])==L"--validate";
 int w=std::stoi(argv[2]),h=std::stoi(argv[3]);if(w<1||h<1||w>4096||h>4096)throw std::runtime_error("Probe size outside 1..4096");auto p=fylm::sg::load(argv[1]);fylm::sg::requireRuntimeSupport(*p);fylm::sg::Settings s;s.virtualHeight=std::stoi(argv[4]);s.frameIndex=std::stoi(argv[5]);s.timeSeconds=s.frameIndex/24.f;retro::RenderRequest r;r.size={w,h};r.timeScale=24;char budgetText[32]{};if(GetEnvironmentVariableA("FYLM_PROBE_BUDGET_MIB",budgetText,sizeof(budgetText))){int budget=std::stoi(budgetText);if(budget<64||budget>1536)throw std::runtime_error("Probe memory budget outside64..1536MiB");r.memoryBudget=std::size_t(budget)*1024*1024;}r.input.resize(std::size_t(w)*h*4);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){auto i=(std::size_t(y)*w+x)*4;r.input[i]=float(x)/std::max(1,w-1);r.input[i+1]=float(y)/std::max(1,h-1);r.input[i+2]=((x/8+y/8)%2)?.8f:.2f;r.input[i+3]=1;}
 if(argc>=9&&!validation){std::ifstream input(std::filesystem::path(argv[8]),std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};if(bytes.size()!=r.input.size())throw std::runtime_error("Input RGBA8 size mismatch");for(std::size_t i=0;i<bytes.size();i++)r.input[i]=bytes[i]/255.f;}
 if(argc==10)s.mix=std::stof(argv[9]);
 int frames=3;char frameText[32]{};if(!validation&&GetEnvironmentVariableA("FYLM_PROBE_FRAMES",frameText,sizeof(frameText))){frames=std::stoi(frameText);if(frames<3||frames>121)throw std::runtime_error("Probe frame count outside3..121");}
 std::vector<double> times;retro::RenderOutput first;for(int n=0;n<frames;n++){auto start=std::chrono::steady_clock::now();auto out=fylm::sg::render(*p,r,s);if(!out.ok)throw std::runtime_error(out.error);times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());if(n==0)first=std::move(out);else if(out.pixels!=first.pixels)throw std::runtime_error("Same-frame replay changed");}
 double energy=0;for(std::size_t i=0;i<first.pixels.size();i++){auto v=first.pixels[i];if(!std::isfinite(v))throw std::runtime_error("Nonfinite rendered output");if(i%4!=3)energy+=std::abs(v);}if(energy<1e-6&&!validation)throw std::runtime_error("Blank RGB output");
 s.frameIndex++;s.timeSeconds=s.frameIndex/24.f;auto next=fylm::sg::render(*p,r,s);if(!next.ok)throw std::runtime_error(next.error);s.frameIndex--;s.timeSeconds=s.frameIndex/24.f;auto replay=fylm::sg::render(*p,r,s);if(!replay.ok||replay.pixels!=first.pixels)throw std::runtime_error("Out-of-order frame replay changed");
 std::ofstream output(std::filesystem::path(argv[6]),std::ios::binary);for(auto v:first.pixels){auto byte=static_cast<unsigned char>(std::lround(std::clamp(v,0.f,1.f)*255));output.write(reinterpret_cast<char*>(&byte),1);}if(!output)throw std::runtime_error("Cannot write probe image");report={{"status","d3d-execution-passed"},{"backend",fylm::sg::rendererBackend()},{"name",p->name},{"width",w},{"height",h},{"virtual_height",s.virtualHeight},{"passes",p->passes.size()},{"memory_budget_mib",r.memoryBudget/1024/1024},{"same_frame",true},{"out_of_order",true},{"finite_nonblank",true},{"identity_default",first.pixels==r.input},{"frame_changes",next.pixels!=first.pixels},{"elapsed_ms",times},{"visual_reference","not-validated"},{"ae","not-tested"}};
 std::vector<double> warm(times.begin()+1,times.end());std::sort(warm.begin(),warm.end());auto percentile=[&](double q){return warm[std::size_t(std::ceil(q*(warm.size()-1)))];};PROCESS_MEMORY_COUNTERS memory{};memory.cb=sizeof(memory);K32GetProcessMemoryInfo(GetCurrentProcess(),&memory,sizeof(memory));
 report["validation_allows_blank"]=validation;report["finite_nonblank"]=energy>=1e-6;report["resources"]=p->resources.size();report["effective_budget_bytes"]=fylm::sg::rendererBudgetBytes();report["planned_working_peak_bytes"]=fylm::sg::rendererPeakBytes();report["probe_process_peak_working_set_bytes"]=memory.PeakWorkingSetSize;report["cold_ms"]=times[0];report["warm_frames"]=warm.size();report["warm_p50_ms"]=percentile(.5);report["warm_p95_ms"]=percentile(.95);report["timing_scope"]="full upload-render-readback; no output cache; process RAM includes diagnostic reference frames";
 }catch(const std::exception& e){report={{"status","failed"},{"error",e.what()},{"planned_working_peak_bytes",fylm::sg::rendererPeakBytes()}};}
 if(argc>=8&&argc<=10){std::ofstream file(std::filesystem::path(argv[7]),std::ios::binary);file<<report.dump(2)<<'\n';}std::cout<<report.dump(2)<<'\n';return report["status"]=="failed"?1:0;
}

