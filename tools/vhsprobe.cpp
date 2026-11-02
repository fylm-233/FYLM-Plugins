#define FYLM_CRT_ONLY 1
#include "../plugin/ae/FYLMParams.hpp"
#include "fylm/backend.hpp"
#include "fylm/ui_values.hpp"
#include "json.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
using nlohmann::json;
int wmain(int argc,wchar_t** argv){try{
 if(argc<7||argc>9){std::cerr<<"Usage: FYLM-vhsprobe settings.json width height seconds output.rgba report.json [input.rgba8] [baseline.rgba]\n";return 2;}
 std::ifstream file{std::filesystem::path(argv[1])};json settings;file>>settings;
 auto value=[&](const char* key)->double{if(settings.contains(key))return settings.at(key);int i=fylm::index(key);if(i<1)throw std::runtime_error(key);return fylm::Params[i-1].value;};
 retro::RenderRequest r;r.size={std::stoi(argv[2]),std::stoi(argv[3])};if(r.size.width<1||r.size.height<1||r.size.width>4096||r.size.height>4096)throw std::runtime_error("Invalid probe size");r.memoryBudget=1536ull*1024*1024;r.timeScale=1000000;r.timeValue=std::llround(std::stod(argv[4])*r.timeScale);r.controls=fylm::crt::controlsFromUi(value);
 r.input.resize(std::size_t(r.size.width)*r.size.height*4);for(int y=0;y<r.size.height;y++)for(int x=0;x<r.size.width;x++){auto i=(std::size_t(y)*r.size.width+x)*4;r.input[i]=float(x)/std::max(1,r.size.width-1);r.input[i+1]=float(y)/std::max(1,r.size.height-1);r.input[i+2]=((x/8+y/8)%2)?.8f:.2f;r.input[i+3]=1;}
 if(argc>=8&&std::wstring(argv[7])!=L"-"){std::ifstream input(std::filesystem::path(argv[7]),std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};if(bytes.size()!=r.input.size())throw std::runtime_error("Input size");for(std::size_t i=0;i<bytes.size();i++)r.input[i]=bytes[i]/255.f;}
 std::vector<double> elapsed;retro::RenderOutput first;for(int i=0;i<3;i++){auto start=std::chrono::steady_clock::now();auto out=fylm::crt::render(r,fylm::crt::signalFromUi(value));if(!out.ok)throw std::runtime_error(out.error);elapsed.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());if(i==0)first=std::move(out);else if(first.pixels!=out.pixels)throw std::runtime_error("Replay differs");}
 auto saved=r.timeValue;r.timeValue+=500000;auto next=fylm::crt::render(r,fylm::crt::signalFromUi(value));r.timeValue=saved;auto replay=fylm::crt::render(r,fylm::crt::signalFromUi(value));if(!next.ok||!replay.ok||first.pixels!=replay.pixels)throw std::runtime_error("Out-of-order differs");
 std::vector<unsigned char> bytes;double energy=0;for(std::size_t i=0;i<first.pixels.size();i++){float v=first.pixels[i];if(!std::isfinite(v))throw std::runtime_error("Nonfinite");if(i%4!=3)energy+=std::abs(v);bytes.push_back(static_cast<unsigned char>(std::lround(std::clamp(v,0.f,1.f)*255)));}if(energy<1e-6)throw std::runtime_error("Blank output");
 if(argc==9){std::ifstream input(std::filesystem::path(argv[8]),std::ios::binary);std::vector<unsigned char> original{std::istreambuf_iterator<char>(input),{}};if(original!=bytes)throw std::runtime_error("Legacy byte baseline differs");}
 std::ofstream out(std::filesystem::path(argv[5]),std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!out)throw std::runtime_error("Cannot save image");
 json report={{"ok",true},{"backend",fylm::crt::rendererBackend()},{"dynamic",next.pixels!=first.pixels},{"same_frame",true},{"out_of_order",true},{"width",r.size.width},{"height",r.size.height},{"elapsed_ms",elapsed},{"legacy_bytes_checked",argc==9},{"ae","not-tested"}};std::ofstream summary(std::filesystem::path(argv[6]),std::ios::binary);summary<<report.dump(2);std::cout<<report.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
