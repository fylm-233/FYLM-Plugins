#include "fylm/backend.hpp"
#include "retro/workspace.hpp"
#include <stdexcept>
#include <cmath>
namespace fylm::crt {
namespace {thread_local std::string activeBackend;}
std::string rendererBackend(){return activeBackend;}
retro::RenderOutput render(const retro::RenderRequest& r,const SignalSettings& settings) noexcept {
 try{
  if(settings.model<0||settings.model>1||settings.seed<0||settings.seed>65535||!std::isfinite(settings.chromaNoise)||settings.chromaNoise<0||settings.chromaNoise>1||!std::isfinite(settings.headSwitch)||settings.headSwitch<0||settings.headSwitch>1||!std::isfinite(settings.strength)||settings.strength<0||settings.strength>1)return {false,"retro.error.range",{}};
  // The shared workspace lease serializes immediate-context access.
  // Cancellation callbacks are outside backend GPU operations.
  if(r.cancelled&&r.cancelled())return {false,"retro.error.cancelled",{}};
  retro::RenderOutput output;
  {
   retro::WorkspaceLease lease;if(!lease.acquire(r.cancelled))return {false,"retro.error.cancelled",{}};
   static std::unique_ptr<IRenderBackend> backend;
   if(!backend)backend=makeD3D11Backend();
   try{
    struct Trim{IRenderBackend* p;~Trim(){p->releaseFrame();}} trim{backend.get()};
    output=backend->render(r,settings);activeBackend=backend->name();
   }catch(...){backend.reset();throw;} // Recreate after device/allocation failure.
  }
  if(r.cancelled&&r.cancelled())return {false,"retro.error.cancelled",{}};
  return output;
 }catch(const std::bad_alloc&){return {false,"retro.error.memory",{}};}
 catch(const std::exception&){return {false,"retro.error.device",{}};}
}
}
