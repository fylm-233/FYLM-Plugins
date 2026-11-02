#define PF_DEEP_COLOR_AWARE 1
#include "gpu_bridge.hpp"
#include "AE_EffectPixelFormat.h"
#include "AE_EffectGPUSuites.h"
#include <windows.h>
#include <GL/gl.h>
#include <stdexcept>
namespace retro::ae {
namespace {
struct Suite {
 PF_InData* in;const PF_GPUDeviceSuite1* p=nullptr;
 explicit Suite(PF_InData* data):in(data){
  if(!in->pica_basicP||in->pica_basicP->AcquireSuite(kPFGPUDeviceSuite,kPFGPUDeviceSuiteVersion1,reinterpret_cast<const void**>(&p))||!p)throw std::runtime_error("retro.error.device");
 }
 ~Suite(){in->pica_basicP->ReleaseSuite(kPFGPUDeviceSuite,kPFGPUDeviceSuiteVersion1);}
};
struct Device {
 Suite& suite;A_u_long index=0;PF_GPUDeviceInfo info{};bool held=false;
 explicit Device(Suite& s):suite(s){
  A_u_long count=0;if(s.p->GetDeviceCount(s.in->effect_ref,&count))throw std::runtime_error("retro.error.device");
  bool found=false;for(A_u_long i=0;i<count;++i){PF_GPUDeviceInfo candidate{};
   if(!s.p->GetDeviceInfo(s.in->effect_ref,i,&candidate)&&candidate.offscreen_opengl_contextPV&&candidate.offscreen_opengl_devicePV){index=i;info=candidate;found=true;break;}}
  if(!found||s.p->AcquireExclusiveDeviceAccess(s.in->effect_ref,index))throw std::runtime_error("retro.error.device");held=true;
 }
 ~Device(){if(held)suite.p->ReleaseExclusiveDeviceAccess(suite.in->effect_ref,index);}
};
struct Current {
 HGLRC oldContext=wglGetCurrentContext();HDC oldDC=wglGetCurrentDC();
 explicit Current(const PF_GPUDeviceInfo& info){if(!wglMakeCurrent(static_cast<HDC>(info.offscreen_opengl_devicePV),static_cast<HGLRC>(info.offscreen_opengl_contextPV)))throw std::runtime_error("retro.error.context");}
 ~Current(){wglMakeCurrent(oldContext?oldDC:nullptr,oldContext);}
};
}
RenderOutput renderWithHostDevice(PF_InData* in,const RenderRequest& request) noexcept {
 try{Suite suite(in);Device device(suite);Current current(device.info);return renderOpenGL(request);}
 catch(const std::bad_alloc&){return {false,"retro.error.memory",{}};}
 catch(const std::exception& e){return {false,e.what(),{}};}
 catch(...){return {false,"retro.error.internal",{}};}
}
}
