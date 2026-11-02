#include "retro/renderer.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
namespace retro { namespace {
constexpr GLenum FBO=0x8D40,DRAW_FBO=0x8CA9,READ_FBO=0x8CA8,COLOR0=0x8CE0,COMPLETE=0x8CD5;
constexpr GLenum TEX0=0x84C0,ACTIVE_TEX=0x84E0,RGBA32F=0x8814,R16UI=0x8234,RED_INTEGER=0x8D94;
constexpr GLenum PACK_BUFFER=0x88EB,UNPACK_BUFFER=0x88EC,PACK_BINDING=0x88ED,UNPACK_BINDING=0x88EF;
constexpr GLenum VERTEX_SHADER=0x8B31,FRAGMENT_SHADER=0x8B30,COMPILE_STATUS=0x8B81,LINK_STATUS=0x8B82;
struct GL {
#define FN(ret,name,...) using name##Fn=ret(APIENTRY*)(__VA_ARGS__);name##Fn name=nullptr;
 FN(GLuint,CreateShader,GLenum) FN(void,ShaderSource,GLuint,GLsizei,const char* const*,const GLint*)
 FN(void,CompileShader,GLuint) FN(void,GetShaderiv,GLuint,GLenum,GLint*)
 FN(void,GetShaderInfoLog,GLuint,GLsizei,GLsizei*,char*) FN(void,DeleteShader,GLuint)
 FN(GLuint,CreateProgram) FN(void,AttachShader,GLuint,GLuint) FN(void,LinkProgram,GLuint)
 FN(void,GetProgramiv,GLuint,GLenum,GLint*) FN(void,GetProgramInfoLog,GLuint,GLsizei,GLsizei*,char*)
 FN(void,DeleteProgram,GLuint) FN(void,UseProgram,GLuint)
 FN(void,GenFramebuffers,GLsizei,GLuint*) FN(void,BindFramebuffer,GLenum,GLuint)
 FN(void,DeleteFramebuffers,GLsizei,const GLuint*) FN(void,FramebufferTexture2D,GLenum,GLenum,GLenum,GLuint,GLint)
 FN(GLenum,CheckFramebufferStatus,GLenum) FN(void,GenVertexArrays,GLsizei,GLuint*)
 FN(void,BindVertexArray,GLuint) FN(void,DeleteVertexArrays,GLsizei,const GLuint*)
 FN(void,ActiveTexture,GLenum) FN(void,BindBuffer,GLenum,GLuint) FN(void,BindSampler,GLuint,GLuint)
 FN(void,ColorMaski,GLuint,GLboolean,GLboolean,GLboolean,GLboolean)
 FN(void,Enablei,GLenum,GLuint) FN(void,Disablei,GLenum,GLuint)
 FN(void,ClampColor,GLenum,GLenum)
 FN(GLint,GetUniformLocation,GLuint,const char*) FN(void,Uniform1f,GLint,GLfloat) FN(void,Uniform1i,GLint,GLint)
 FN(void,Uniform2f,GLint,GLfloat,GLfloat) FN(void,Uniform2i,GLint,GLint,GLint)
 FN(void,ClipControl,GLenum,GLenum)
#undef FN
 template<class T>static T load(const char* name,bool required=true){
  auto p=wglGetProcAddress(name);auto value=reinterpret_cast<std::intptr_t>(p);
  if(!p||value==1||value==2||value==3||value==-1)p=reinterpret_cast<PROC>(GetProcAddress(GetModuleHandleW(L"opengl32.dll"),name));
  if(!p&&required)throw std::runtime_error("retro.error.gl_version");return reinterpret_cast<T>(p);
 }
 GL(){
  if(!wglGetCurrentContext())throw std::runtime_error("retro.error.context");
  const char* version=reinterpret_cast<const char*>(glGetString(GL_VERSION));int major=0,minor=0;
  if(!version||sscanf_s(version,"%d.%d",&major,&minor)!=2||major<3||(major==3&&minor<3))throw std::runtime_error("retro.error.gl_version");
#define LOAD(name) name=load<name##Fn>("gl" #name);
  LOAD(CreateShader) LOAD(ShaderSource) LOAD(CompileShader) LOAD(GetShaderiv) LOAD(GetShaderInfoLog) LOAD(DeleteShader)
  LOAD(CreateProgram) LOAD(AttachShader) LOAD(LinkProgram) LOAD(GetProgramiv) LOAD(GetProgramInfoLog) LOAD(DeleteProgram) LOAD(UseProgram)
  LOAD(GenFramebuffers) LOAD(BindFramebuffer) LOAD(DeleteFramebuffers) LOAD(FramebufferTexture2D) LOAD(CheckFramebufferStatus)
  LOAD(GenVertexArrays) LOAD(BindVertexArray) LOAD(DeleteVertexArrays) LOAD(ActiveTexture) LOAD(BindBuffer) LOAD(BindSampler)
  LOAD(ColorMaski) LOAD(Enablei) LOAD(Disablei) LOAD(ClampColor)
  LOAD(GetUniformLocation) LOAD(Uniform1f) LOAD(Uniform1i) LOAD(Uniform2f) LOAD(Uniform2i)
#undef LOAD
  if(major>4||(major==4&&minor>=5))ClipControl=load<ClipControlFn>("glClipControl",false);
 }
};
struct State {
 GL& g;GLint program=0,vao=0,draw=0,read=0,active=0,viewport[4]{},packBuffer=0,unpackBuffer=0;
 std::array<GLint,4> textures{},samplers{};std::array<GLint,10> pixels{};bool coreProfile=false;
 GLboolean mask[4]{},blend=false,mapColor=false;GLint polygon[2]{},clipOrigin=0,clipDepth=0,readClamp=0,fragmentClamp=0;
 std::array<GLfloat,8> transfers{};
 std::array<GLboolean,32> clips{};GLint clipCount=0;
 static constexpr GLenum flags[]={GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_DITHER,0x8DB9,0x8C89};
 static constexpr GLenum transferNames[]={GL_RED_SCALE,GL_GREEN_SCALE,GL_BLUE_SCALE,GL_ALPHA_SCALE,GL_RED_BIAS,GL_GREEN_BIAS,GL_BLUE_BIAS,GL_ALPHA_BIAS};
 static constexpr GLenum pixelNames[]={GL_PACK_ALIGNMENT,GL_UNPACK_ALIGNMENT,0x0D02,0x0CF2,0x0D03,0x0CF3,0x0D04,0x0CF4,0x0D00,0x0CF0};
 std::array<GLboolean,7> enabled{};
 explicit State(GL& api):g(api){
  glGetIntegerv(0x0D32,&clipCount);if(clipCount<0||clipCount>int(clips.size()))throw std::runtime_error("retro.error.gl_version");
  for(int i=0;i<clipCount;++i)clips[i]=glIsEnabled(0x3000+i);
  glGetIntegerv(0x8B8D,&program);glGetIntegerv(0x85B5,&vao);glGetIntegerv(0x8CA6,&draw);glGetIntegerv(0x8CAA,&read);
  glGetIntegerv(ACTIVE_TEX,&active);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(PACK_BINDING,&packBuffer);glGetIntegerv(UNPACK_BINDING,&unpackBuffer);
  glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetIntegerv(GL_POLYGON_MODE,polygon);
  blend=glIsEnabled(GL_BLEND);glGetIntegerv(0x891C,&readClamp);g.ClampColor(0x891C,GL_FALSE);
  GLint profile=0;glGetIntegerv(0x9126,&profile);coreProfile=(profile&1)!=0;
  if(!coreProfile){glGetIntegerv(0x891B,&fragmentClamp);g.ClampColor(0x891B,GL_FALSE);glGetBooleanv(GL_MAP_COLOR,&mapColor);glPixelTransferi(GL_MAP_COLOR,GL_FALSE);for(std::size_t i=0;i<transfers.size();++i){glGetFloatv(transferNames[i],&transfers[i]);glPixelTransferf(transferNames[i],i<4?1.f:0.f);}}
  for(std::size_t i=0;i<enabled.size();++i)enabled[i]=glIsEnabled(flags[i]);
  for(std::size_t i=0;i<pixels.size();++i)glGetIntegerv(pixelNames[i],&pixels[i]);
  for(int i=0;i<4;++i){g.ActiveTexture(TEX0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&textures[i]);glGetIntegerv(0x8919,&samplers[i]);}
  if(g.ClipControl){glGetIntegerv(0x935C,&clipOrigin);glGetIntegerv(0x935D,&clipDepth);g.ClipControl(0x8CA1,0x935E);}
  for(auto f:flags)glDisable(f);g.Disablei(GL_BLEND,0);g.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  g.BindBuffer(PACK_BUFFER,0);g.BindBuffer(UNPACK_BUFFER,0);
  for(std::size_t i=0;i<pixels.size();++i)glPixelStorei(pixelNames[i],i<2?1:0);
  for(int i=0;i<4;++i)g.BindSampler(i,0);
  for(int i=0;i<clipCount;++i)glDisable(0x3000+i);
 }
 ~State(){
  g.UseProgram(program);g.BindVertexArray(vao);g.BindFramebuffer(DRAW_FBO,draw);g.BindFramebuffer(READ_FBO,read);
  glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);g.ColorMaski(0,mask[0],mask[1],mask[2],mask[3]);if(blend)g.Enablei(GL_BLEND,0);else g.Disablei(GL_BLEND,0);
  g.ClampColor(0x891C,readClamp);
  if(!coreProfile){g.ClampColor(0x891B,fragmentClamp);glPixelTransferi(GL_MAP_COLOR,mapColor);for(std::size_t i=0;i<transfers.size();++i)glPixelTransferf(transferNames[i],transfers[i]);}
  if(coreProfile)glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);else{glPolygonMode(GL_FRONT,polygon[0]);glPolygonMode(GL_BACK,polygon[1]);}
  for(std::size_t i=0;i<enabled.size();++i){if(enabled[i])glEnable(flags[i]);else glDisable(flags[i]);}
  for(std::size_t i=0;i<pixels.size();++i)glPixelStorei(pixelNames[i],pixels[i]);
  g.BindBuffer(PACK_BUFFER,packBuffer);g.BindBuffer(UNPACK_BUFFER,unpackBuffer);
  for(int i=0;i<4;++i){g.ActiveTexture(TEX0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);g.BindSampler(i,samplers[i]);}
  g.ActiveTexture(active);if(g.ClipControl)g.ClipControl(clipOrigin,clipDepth);
  for(int i=0;i<clipCount;++i){if(clips[i])glEnable(0x3000+i);else glDisable(0x3000+i);}
 }
};
UniformApi apiFor(GL& g){return {&g,
 [](void*p,std::uint32_t program,const char*n){return static_cast<GL*>(p)->GetUniformLocation(program,n);},
 [](void*p,int l,float v){static_cast<GL*>(p)->Uniform1f(l,v);},
 [](void*p,int l,int v){static_cast<GL*>(p)->Uniform1i(l,v);},
 [](void*p,int l,float x,float y){static_cast<GL*>(p)->Uniform2f(l,x,y);},
 [](void*p,int l,int x,int y){static_cast<GL*>(p)->Uniform2i(l,x,y);}};}
struct Texture {GLuint id=0;Size size;bool live=false;std::size_t bytes=0;};
struct Engine {
 GL g;State state;GLuint fbo=0,vao=0,palette=0,ranks=0;
 std::array<GLuint,7> programs{};std::array<UniformLocations,7> locations;
 std::vector<Texture> textures;std::size_t allocated=0;const RenderRequest& request;GLint maxSize=0;
 explicit Engine(const RenderRequest& r):state(g),request(r){
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxSize);g.GenFramebuffers(1,&fbo);g.GenVertexArrays(1,&vao);
 }
 ~Engine(){glFinish();for(auto p:programs)if(p)g.DeleteProgram(p);for(auto t:textures)if(t.id)glDeleteTextures(1,&t.id);
  if(palette)glDeleteTextures(1,&palette);if(ranks)glDeleteTextures(1,&ranks);if(fbo)g.DeleteFramebuffers(1,&fbo);if(vao)g.DeleteVertexArrays(1,&vao);}
 void abort(){if(request.cancelled&&request.cancelled())throw std::runtime_error("retro.error.cancelled");}
 GLuint compile(GLenum type,const std::string& text){
  GLuint s=g.CreateShader(type);if(!s)throw std::runtime_error("retro.error.shader");const char* p=text.c_str();g.ShaderSource(s,1,&p,nullptr);g.CompileShader(s);GLint ok=0;g.GetShaderiv(s,COMPILE_STATUS,&ok);
  if(!ok){g.DeleteShader(s);throw std::runtime_error("retro.error.shader");}return s;
 }
 GLuint program(int variant){
  if(programs[variant])return programs[variant];GLuint vs=0,fs=0,p=0;
  try{vs=compile(VERTEX_SHADER,vertexShader());fs=compile(FRAGMENT_SHADER,fragmentShader(variant));p=g.CreateProgram();g.AttachShader(p,vs);g.AttachShader(p,fs);g.LinkProgram(p);GLint ok=0;g.GetProgramiv(p,LINK_STATUS,&ok);if(!ok)throw std::runtime_error("retro.error.shader");g.DeleteShader(vs);vs=0;g.DeleteShader(fs);fs=0;locations[variant]=locateUniforms(apiFor(g),p);programs[variant]=p;return p;}
  catch(...){if(vs)g.DeleteShader(vs);if(fs)g.DeleteShader(fs);if(p)g.DeleteProgram(p);throw;}
 }
 void bind(GLuint unit,GLuint id,bool nearest=false){g.ActiveTexture(TEX0+unit);glBindTexture(GL_TEXTURE_2D,id);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,nearest?GL_NEAREST:GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,nearest?GL_NEAREST:GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);}
 std::size_t count(Size s){if(s.width<=0||s.height<=0||s.width>maxSize||s.height>maxSize||size_t(s.width)>SIZE_MAX/size_t(s.height)/16)throw std::runtime_error("retro.error.resolution");return size_t(s.width)*s.height*4;}
 std::size_t acquire(Size size){
  for(std::size_t i=0;i<textures.size();++i)if(!textures[i].live&&textures[i].size.width==size.width&&textures[i].size.height==size.height){textures[i].live=true;return i;}
  auto bytes=count(size)*sizeof(float);if(bytes>request.memoryBudget||allocated>request.memoryBudget-bytes)throw std::runtime_error("retro.error.memory");
  Texture t;t.size=size;t.live=true;t.bytes=bytes;glGenTextures(1,&t.id);if(!t.id)throw std::runtime_error("retro.error.memory");
  bind(0,t.id);glTexImage2D(GL_TEXTURE_2D,0,RGBA32F,size.width,size.height,0,GL_RGBA,GL_FLOAT,nullptr);
  GLint actualWidth=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&actualWidth);if(actualWidth!=size.width){glDeleteTextures(1,&t.id);throw std::runtime_error("retro.error.memory");}
  try{textures.push_back(t);}catch(...){glDeleteTextures(1,&t.id);throw;}allocated+=bytes;return textures.size()-1;
 }
 void target(std::size_t index){g.BindFramebuffer(FBO,fbo);g.FramebufferTexture2D(FBO,COLOR0,GL_TEXTURE_2D,textures[index].id,0);glDrawBuffer(COLOR0);glReadBuffer(COLOR0);if(g.CheckFramebufferStatus(FBO)!=COMPLETE)throw std::runtime_error("retro.error.framebuffer");auto s=textures[index].size;glViewport(0,0,s.width,s.height);g.BindVertexArray(vao);}
 void auxiliary(){
  glGenTextures(1,&palette);bind(1,palette,true);int n=std::max(1,request.controls.dither.paletteSize);const float black[3]={0,0,0};
  glTexImage2D(GL_TEXTURE_2D,0,0x8815,n,1,0,GL_RGB,GL_FLOAT,request.palette.empty()?black:request.palette.data());
  GLint width=0;glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);if(width!=n)throw std::runtime_error("retro.error.memory");
  glGenTextures(1,&ranks);bind(2,ranks,true);glTexImage2D(GL_TEXTURE_2D,0,R16UI,64,64,0,RED_INTEGER,GL_UNSIGNED_SHORT,blueNoiseRanking());
  glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);if(width!=64)throw std::runtime_error("retro.error.memory");
 }
 std::size_t upload(const std::vector<float>& data,Size size){if(data.size()!=count(size))throw std::runtime_error("retro.error.frame");auto id=acquire(size);bind(0,textures[id].id);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,size.width,size.height,GL_RGBA,GL_FLOAT,data.data());return id;}
 std::vector<float> download(std::size_t index){std::vector<float> pixels(count(textures[index].size));target(index);auto s=textures[index].size;glReadPixels(0,0,s.width,s.height,GL_RGBA,GL_FLOAT,pixels.data());return pixels;}
 std::size_t draw(std::size_t source,int variant,Size size,std::size_t alpha=SIZE_MAX,bool nearest=false){
  abort();auto dest=acquire(size);auto p=program(variant);target(dest);g.UseProgram(p);
  bind(0,textures[source].id,nearest);bind(1,palette,true);bind(2,ranks,true);bind(3,textures[alpha==SIZE_MAX?source:alpha].id,nearest);
  auto controls=request.controls;if(controls.dither.algorithmId==1&&request.cpuDither)controls.dither.enabled=0;
  auto f=makeUniformFrame(controls,size,request.timeValue,request.timeScale);bindUniforms(apiFor(g),locations[variant],f);
  auto one=[&](const char* n,float v){auto l=g.GetUniformLocation(p,n);if(l>=0)g.Uniform1f(l,v);};
  auto a=g.GetUniformLocation(p,"uOriginalAlpha");if(a>=0)g.Uniform1i(a,3);
  one("uToneContrast",request.controls.toneContrast);one("uToneMidtones",request.controls.toneMidtones);one("uToneHighlights",request.controls.toneHighlights);
  glDrawArrays(GL_TRIANGLES,0,3);return dest;
 }
 std::vector<float> run(){
  abort();auxiliary();auto current=upload(request.input,request.size);auto c=request.controls;
  auto replace=[&](std::size_t dest,std::size_t pin=SIZE_MAX){if(current!=pin)textures[current].live=false;current=dest;};
  auto analog=[&]{if(!c.retroEnabled)return;if(c.crt.vhsEnabled)replace(draw(current,2,request.size));if(c.crt.crtEnabled)replace(draw(current,3,request.size));};
  auto dither=[&]{if(!c.dither.enabled)return;
   if(c.dither.algorithmId==1){if(!request.cpuDither)throw std::runtime_error("retro.error.cpu_algorithm");auto data=download(current);abort();auto result=request.cpuDither(data);replace(upload(result,request.size));return;}
   std::size_t original=current;bool preserve=c.dither.preserveAlpha!=0;
   double factor=c.workingResolution>0?std::min(1.0,double(c.workingResolution)/std::max(request.size.width,request.size.height)):1;
   Size work{std::max(1,int(std::floor(request.size.width*factor+.5))),std::max(1,int(std::floor(request.size.height*factor+.5)))};
   auto pin=preserve?original:SIZE_MAX;
   if(work.width!=request.size.width||work.height!=request.size.height)replace(draw(current,0,work),pin);
   replace(draw(current,5,work),pin);replace(draw(current,1,work),pin);
   if(work.width!=request.size.width||work.height!=request.size.height)replace(draw(current,0,request.size,SIZE_MAX,true),pin);
   if(preserve){replace(draw(current,6,request.size,original));textures[original].live=false;}
  };
  if(c.order==Order::DitherThenRetro){dither();analog();}else{analog();dither();}
  abort();auto result=download(current);abort();return result;
 }
};
} // namespace
RenderOutput renderOpenGL(const RenderRequest& r) noexcept {try{
 if(r.size.width<=0||r.size.height<=0||!r.timeScale||size_t(r.size.width)>SIZE_MAX/size_t(r.size.height)/4||r.input.size()!=size_t(r.size.width)*r.size.height*4)return {false,"retro.error.frame",{}};
 Controls check=r.controls;if(check.dither.enabled&&check.dither.algorithmId==1&&r.cpuDither)check.dither.enabled=0;
 std::string error;if(!validate(check,error))return {false,error,{}};
 if(!std::isfinite(r.controls.toneContrast)||!std::isfinite(r.controls.toneMidtones)||!std::isfinite(r.controls.toneHighlights))return {false,"retro.error.nonfinite",{}};
 if(r.controls.dither.paletteSize>0&&r.palette.size()!=size_t(r.controls.dither.paletteSize)*3)return {false,"retro.error.dither",{}};
 for(float value:r.palette)if(!std::isfinite(value)||value<0||value>1)return {false,"retro.error.dither",{}};
 Engine engine(r);auto pixels=engine.run();return {true,{},std::move(pixels)};
 }catch(const std::bad_alloc&){return {false,"retro.error.memory",{}};}catch(const std::exception& e){return {false,e.what(),{}};}catch(...){return {false,"retro.error.internal",{}};}}
}
