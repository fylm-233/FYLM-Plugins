#include "retro/renderer.hpp"
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <stdexcept>
#include <thread>
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
struct Context {
 HWND window=nullptr;HDC dc=nullptr;HGLRC rc=nullptr;std::mutex mutex;
 Context(){WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"RetroGpuTestOnly";RegisterClassW(&wc);
  window=CreateWindowW(wc.lpszClassName,L"",WS_POPUP,0,0,32,32,nullptr,nullptr,wc.hInstance,nullptr);check(window!=nullptr,"test window");dc=GetDC(window);
  PIXELFORMATDESCRIPTOR p{};p.nSize=sizeof(p);p.nVersion=1;p.dwFlags=PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW;p.iPixelType=PFD_TYPE_RGBA;p.cColorBits=32;p.iLayerType=PFD_MAIN_PLANE;
  int format=ChoosePixelFormat(dc,&p);check(format&&SetPixelFormat(dc,format,&p),"pixel format");rc=wglCreateContext(dc);check(rc&&wglMakeCurrent(dc,rc),"GL context");
  std::cout<<"GL="<<glGetString(GL_VERSION)<<" GPU="<<glGetString(GL_RENDERER)<<'\n';wglMakeCurrent(nullptr,nullptr);
 }
 ~Context(){wglMakeCurrent(nullptr,nullptr);if(rc)wglDeleteContext(rc);if(dc)ReleaseDC(window,dc);if(window)DestroyWindow(window);UnregisterClassW(L"RetroGpuTestOnly",GetModuleHandleW(nullptr));}
 retro::RenderOutput render(const retro::RenderRequest& r){std::lock_guard<std::mutex> lock(mutex);check(wglMakeCurrent(dc,rc),"worker context");
  glViewport(3,4,9,10);glEnable(GL_BLEND);glEnable(GL_CLIP_PLANE0);glPixelTransferf(GL_RED_SCALE,.5f);glPixelStorei(0x0D00,GL_TRUE);glPixelStorei(0x0CF2,7);
  auto output=retro::renderOpenGL(r);GLint viewport[4]{};glGetIntegerv(GL_VIEWPORT,viewport);GLint swap=0,row=0;glGetIntegerv(0x0D00,&swap);glGetIntegerv(0x0CF2,&row);
  GLfloat scale=0;glGetFloatv(GL_RED_SCALE,&scale);
  bool restored=viewport[0]==3&&viewport[1]==4&&viewport[2]==9&&viewport[3]==10&&glIsEnabled(GL_BLEND)&&glIsEnabled(GL_CLIP_PLANE0)&&scale==.5f&&swap==GL_TRUE&&row==7;
  GLenum error=glGetError();wglMakeCurrent(nullptr,nullptr);check(restored,"GL state leaked");check(error==GL_NO_ERROR,"GL error");return output;
 }
};
retro::RenderRequest frame(int n){retro::RenderRequest r;r.size={32,24};r.timeValue=n*1001;r.timeScale=24000;r.input.resize(32*24*4);
 for(int y=0;y<24;y++)for(int x=0;x<32;x++){int i=(y*32+x)*4;r.input[i]=((x*17+y*7+n*11)%256)/255.f;r.input[i+1]=((x*5+y*19+n*3)%256)/255.f;r.input[i+2]=((x*13+y*11+n*23)%256)/255.f;r.input[i+3]=((x+y)%7)/6.f;}
 constexpr int ids[]={2,8,15,17};r.controls.retroEnabled=true;r.controls.dither.algorithmId=ids[n%4];r.controls.order=n%2?retro::Order::DitherThenRetro:retro::Order::RetroThenDither;
 r.controls.workingResolution=n%3?16:0;r.controls.dither.preserveAlpha=1;r.controls.toneContrast=12;r.controls.toneMidtones=.9f;r.controls.toneHighlights=20;
 r.controls.crt.curvature=.04f;return r;}
bool equal(const std::vector<float>& a,const std::vector<float>& b){return a==b;}
int main(){try{Context ctx;
 auto identity=frame(0);identity.controls.retroEnabled=false;identity.controls.dither.enabled=0;auto out=ctx.render(identity);check(out.ok&&equal(out.pixels,identity.input),"identity");
 auto analogOnly=frame(0);analogOnly.controls.dither.enabled=0;out=ctx.render(analogOnly);check(out.ok,"analog without dither");
 auto crtOnly=frame(0);crtOnly.controls.dither.enabled=0;crtOnly.controls.crt.vhsEnabled=0;out=ctx.render(crtOnly);check(out.ok,"CRT without dither");
 auto vhsOnly=frame(0);vhsOnly.controls.dither.enabled=0;vhsOnly.controls.crt.crtEnabled=0;out=ctx.render(vhsOnly);check(out.ok,"VHS without dither");
 auto motion0=frame(0),motion1=frame(0);motion0.controls.dither.enabled=0;motion1.controls.dither.enabled=0;motion1.timeValue+=1001;
 auto motionFrame0=ctx.render(motion0),motionFrame1=ctx.render(motion1);check(motionFrame0.ok&&motionFrame1.ok&&!equal(motionFrame0.pixels,motionFrame1.pixels),"dynamic VHS time");
 auto alpha=frame(0);alpha.controls.retroEnabled=false;alpha.controls.workingResolution=8;out=ctx.render(alpha);check(out.ok,"alpha render");for(size_t i=3;i<out.pixels.size();i+=4)check(std::abs(out.pixels[i]-alpha.input[i])<1e-6,"full size alpha lost");
 auto cancelled=frame(0);cancelled.cancelled=[](){return true;};out=ctx.render(cancelled);check(!out.ok&&out.error=="retro.error.cancelled","cancel");
 auto budget=frame(0);budget.memoryBudget=16;out=ctx.render(budget);check(!out.ok&&out.error=="retro.error.memory","memory cap");
 std::vector<std::vector<float>> expected(100);for(int n=0;n<100;n++){auto r=ctx.render(frame(n));check(r.ok,r.error.c_str());expected[n]=std::move(r.pixels);}
 std::vector<int> order(100);std::iota(order.begin(),order.end(),0);std::mt19937 rng(42);std::shuffle(order.begin(),order.end(),rng);
 std::atomic<int> next{0};std::atomic<bool> failed{false};std::vector<std::thread> workers;
 for(int t=0;t<4;t++)workers.emplace_back([&]{try{for(;;){int k=next.fetch_add(1);if(k>=100)break;int n=order[k];auto r=ctx.render(frame(n));if(!r.ok||!equal(r.pixels,expected[n]))failed=true;}}catch(...){failed=true;}});
 for(auto& t:workers)t.join();check(!failed,"100-frame order/concurrency contamination");
 std::cout<<"Actual GPU: independent dither/analog combinations, dynamic VHS time, identity, alpha, cancellation, budget, 100 sequential vs shuffled 4-worker frames bit-identical.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
