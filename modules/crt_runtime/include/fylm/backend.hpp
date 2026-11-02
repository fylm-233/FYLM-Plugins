#pragma once
#include "retro/renderer.hpp"
#include <memory>
namespace fylm::crt {
// Native D3D tape extension; separate from the legacy portable GLSL contract.
struct SignalSettings {int model=0,seed=0;float chromaNoise=0,headSwitch=0,strength=1;};
// CPU staging boundary. No Adobe or platform API types cross this interface.
class IRenderBackend {
public:
 virtual ~IRenderBackend()=default;
 virtual retro::RenderOutput render(const retro::RenderRequest&,const SignalSettings& = {})=0;
 virtual const char* name()const noexcept=0;
 virtual void releaseFrame()noexcept=0;
};
std::unique_ptr<IRenderBackend> makeD3D11Backend();
std::string rendererBackend();
retro::RenderOutput render(const retro::RenderRequest&,const SignalSettings& = {}) noexcept;
}
