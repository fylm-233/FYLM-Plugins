#include "fylm/backend.hpp"
#include "preset.hpp"
#include "retro/workspace.hpp"
#include <iostream>
#include <stdexcept>
#include <future>
int main(int argc,char** argv){try{
 if(argc!=2)return 2;retro::RenderRequest r;r.size={3840,2160};r.input.assign(std::size_t(3840)*2160*4,.4f);r.timeScale=24;r.memoryBudget=retro::WorkspaceLease::limit;r.controls.retroEnabled=true;
 {auto out=fylm::crt::render(r);if(!out.ok||out.pixels.size()!=r.input.size())throw std::runtime_error("4K CRT: "+out.error);}
 auto preset=fylm::sg::load(argv[1]);fylm::sg::Settings settings;settings.virtualHeight=240;
 {auto out=fylm::sg::render(*preset,r,settings);if(!out.ok||out.pixels.size()!=r.input.size())throw std::runtime_error("4K ShaderGlass: "+out.error);}
 r.memoryBudget=1;if(fylm::sg::render(*preset,r,settings).error!="sg.error.budget")throw std::runtime_error("budget rejection");
 // Waiters remain cancellable and a lease is shared across worker instances.
 {retro::WorkspaceLease owner;if(!owner.acquire({}))throw std::runtime_error("owner lease");
  auto waiting=std::async(std::launch::async,[]{retro::WorkspaceLease other;return other.acquire([]{return true;});});if(waiting.get())throw std::runtime_error("cancelled waiter acquired");}
 std::cout<<"4K CRT and ShaderGlass staging, budget rejection and cancellable workspace passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
