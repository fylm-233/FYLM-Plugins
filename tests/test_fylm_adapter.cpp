#include "../plugin/ae/FYLMEffects.cpp"
#include <iostream>
#include <set>
struct Host {PF_ParamDef p[fylm::Count]{};PF_EffectWorld src{},dst{};int added=0,checked=0,checkins=0;bool cancel=false;};
Host& host(PF_ProgPtr p){return *reinterpret_cast<Host*>(p);}
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
PF_Err add(PF_ProgPtr r,PF_ParamIndex,PF_ParamDef* p){auto& h=host(r);h.p[++h.added]=*p;if(p->param_type==PF_Param_ARBITRARY_DATA)h.p[h.added].u.arb_d.value=p->u.arb_d.dephault;return 0;}
PF_Err checkout(PF_ProgPtr r,PF_ParamIndex i,A_long,A_long,A_u_long,PF_ParamDef* p){*p=host(r).p[i];return 0;}
PF_Err checkin(PF_ProgPtr r,PF_ParamDef*){++host(r).checkins;return 0;}
PF_Err aborter(PF_ProgPtr r){return host(r).cancel?PF_Interrupt_CANCEL:0;}
std::set<A_long> checkoutIds;
PF_Err layer(PF_ProgPtr r,PF_ParamIndex,A_long id,const PF_RenderRequest*,A_long,A_long,A_u_long,PF_CheckoutResult* result){if(!checkoutIds.insert(id).second)return PF_Err_BAD_CALLBACK_PARAM;auto& s=host(r).src;result->result_rect=result->max_result_rect={s.origin_x,s.origin_y,s.origin_x+s.width,s.origin_y+s.height};return 0;}
PF_Err pixels(PF_ProgPtr r,A_long id,PF_EffectWorld** p){if(checkoutIds.count(id)==0)return PF_Err_INVALID_INDEX;++host(r).checked;*p=&host(r).src;return 0;}
PF_Err checkin(PF_ProgPtr r,A_long){--host(r).checked;return 0;}
PF_Err output(PF_ProgPtr r,PF_EffectWorld** p){*p=&host(r).dst;return 0;}
using EffectEntry=decltype(&EffectMain);
EffectEntry binaryEntry=&EffectMain;
PF_Err dispatch(PF_Cmd cmd,PF_InData* in,PF_OutData* out,PF_ParamDef* p[],PF_LayerDef* layer,void* extra){if(cmd==PF_Cmd_SMART_PRE_RENDER)checkoutIds.clear();return binaryEntry(cmd,in,out,p,layer,extra);}
#define EffectMain dispatch
#include "pixel_art_preset_host_checks.hpp"
#include "shaderglass_preset_host_checks.hpp"
#include "vhs_preset_host_checks.hpp"
#include "shaderglass_import_host_checks.hpp"
int main(int argc,char** argv){try{
 if(argc>1){
  HMODULE module=LoadLibraryA(argv[1]);require(module!=nullptr,"load actual aex");
  binaryEntry=reinterpret_cast<EffectEntry>(GetProcAddress(module,"EffectMain"));require(binaryEntry!=nullptr,"EffectMain export");
  require(GetProcAddress(module,"PluginDataEntryFunction2")!=nullptr,"registration export");
  auto resource=FindResourceW(module,MAKEINTRESOURCEW(16000),L"PiPL");require(resource!=nullptr,"PiPL missing");
  auto bytes=static_cast<const char*>(LockResource(LoadResource(module,resource)));auto size=SizeofResource(module,resource);
  std::string pipl(bytes,bytes+size);require(pipl.find(fylm::Name)!=std::string::npos&&pipl.find(fylm::Category)!=std::string::npos&&pipl.find(fylm::Match)!=std::string::npos,"binary PiPL identity/category");
  auto version=pipl.find("REVe");require(version!=std::string::npos&&version+16<=pipl.size(),"PiPL version property");std::uint32_t packed=0;std::memcpy(&packed,pipl.data()+version+12,4);require(packed==FYLM_PACKED_VERSION,"PiPL version differs from runtime configuration");
 }
 Host h;PF_InData in{};PF_OutData out{};in.effect_ref=reinterpret_cast<PF_ProgPtr>(&h);in.inter.add_param=add;in.inter.checkout_param=checkout;in.inter.checkin_param=checkin;in.inter.abort=aborter;
 // Every pre-render must return all checked-out parameters before it returns.
 auto leases=[&](const char* stage){require(h.checkins==fylm::Count-1,stage);h.checkins=0;};
#if !defined(FYLM_CRT_ONLY)
 PF_UtilCallbacks utils{};in.utils=&utils;
 utils.host_new_handle=[](A_u_longlong n)->PF_Handle{auto raw=new unsigned char[std::size_t(n)+sizeof(A_u_longlong)];std::memcpy(raw,&n,sizeof(n));auto h=new char*;*h=reinterpret_cast<char*>(raw+sizeof(n));return reinterpret_cast<PF_Handle>(h);};
 utils.host_lock_handle=[](PF_Handle h)->void*{return *h;};utils.host_unlock_handle=[](PF_Handle){};
 utils.host_get_handle_size=[](PF_Handle h)->A_u_longlong{A_u_longlong n;std::memcpy(&n,reinterpret_cast<unsigned char*>(*h)-sizeof(n),sizeof(n));return n;};
 utils.host_dispose_handle=[](PF_Handle h){delete[] (reinterpret_cast<unsigned char*>(*h)-sizeof(A_u_longlong));delete h;};
#endif
 require(EffectMain(PF_Cmd_PARAMS_SETUP,&in,&out,nullptr,nullptr,nullptr)==0,"registration");
 require(h.added==fylm::Count-1&&out.num_params==fylm::Count,"parameter count");
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
 auto paletteBytes=paEncode({{255,0,0},{255,255,255}});PF_ArbitraryH importedPalette=nullptr;PF_ArbParamsExtra paletteArb{};paletteArb.which_function=PF_Arbitrary_UNFLATTEN_FUNC;paletteArb.u.unflatten_func_params.buf_sizeLu=A_u_long(paletteBytes.size());paletteArb.u.unflatten_func_params.flat_dataPV=paletteBytes.data();paletteArb.u.unflatten_func_params.arbPH=&importedPalette;
 require(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&in,&out,nullptr,nullptr,&paletteArb)==0,"palette deserialize");h.p[fylm::index("pa.data")].u.arb_d.value=importedPalette;h.p[fylm::index("pa.palette")].u.pd.value=2;
 std::vector<unsigned char> flattenedPalette(paletteBytes.size());paletteArb={};paletteArb.which_function=PF_Arbitrary_FLATTEN_FUNC;paletteArb.u.flatten_func_params.arbH=importedPalette;paletteArb.u.flatten_func_params.buf_sizeLu=A_u_long(flattenedPalette.size());paletteArb.u.flatten_func_params.flat_dataPV=flattenedPalette.data();require(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&in,&out,nullptr,nullptr,&paletteArb)==0&&flattenedPalette==paletteBytes,"palette persistence round trip");
#endif
#ifdef FYLM_SHADERGLASS
 auto embedded=fylm::sg::builtinBytes(2);require(!embedded.empty(),"embedded source fixture");
 PF_ArbitraryH imported=nullptr;PF_ArbParamsExtra arb{};arb.which_function=PF_Arbitrary_UNFLATTEN_FUNC;arb.u.unflatten_func_params.buf_sizeLu=A_u_long(embedded.size());arb.u.unflatten_func_params.flat_dataPV=embedded.data();arb.u.unflatten_func_params.arbPH=&imported;
 require(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&in,&out,nullptr,nullptr,&arb)==0,"preset deserialize");
 A_u_long flatSize=0;arb={};arb.which_function=PF_Arbitrary_FLAT_SIZE_FUNC;arb.u.flat_size_func_params.arbH=imported;arb.u.flat_size_func_params.flat_data_sizePLu=&flatSize;require(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&in,&out,nullptr,nullptr,&arb)==0&&flatSize==embedded.size(),"preset flat size");
 std::vector<unsigned char> flattened(flatSize);arb={};arb.which_function=PF_Arbitrary_FLATTEN_FUNC;arb.u.flatten_func_params.arbH=imported;arb.u.flatten_func_params.buf_sizeLu=flatSize;arb.u.flatten_func_params.flat_dataPV=flattened.data();require(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&in,&out,nullptr,nullptr,&arb)==0&&flattened==embedded,"preset save roundtrip");
 h.p[fylm::index("sg.data")].u.arb_d.value=imported;
#endif

#ifdef FYLM_SHADERGLASS
 sgHostDefaults(h,in,out);
 sgHostImport(h,in,out);
#elif defined(FYLM_CRT_ONLY)
 vhsHostPresets(h,in,out);
#endif
 require(std::strcmp(fylm::Category,"FYLM Plugins")==0,"category");
 require(EffectMain(PF_Cmd_GLOBAL_SETUP,&in,&out,nullptr,nullptr,nullptr)==0,"global setup");
 require(out.my_version==FYLM_PACKED_VERSION,"runtime packed version");
#if defined(FYLM_SHADERGLASS)
 require((out.out_flags&PF_OutFlag_NON_PARAM_VARY)!=0,"ShaderGlass time flag");
 require(fylm::index("sg.preset")>0&&fylm::index("retro.vhs")==-1,"ShaderGlass UI identity");
#elif defined(FYLM_CRT_ONLY)
 require((out.out_flags&PF_OutFlag_NON_PARAM_VARY)!=0,"missing time flag");
 require(fylm::index("param.algorithm")==-1&&fylm::index("retro.vhs")>0,"CRT UI contains dither");
#else
 require((out.out_flags&PF_OutFlag_NON_PARAM_VARY)==0,"dither shouldn't be temporal");
 require(fylm::index("retro.vhs")==-1&&h.p[fylm::index("param.algorithm")].u.pd.num_choices==9,"dither UI split/halftone");
#endif
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
 for(int depth:{8,16,32}){
#else
 for(int depth:{8,16}){
#endif
  int w=96,height=96,bpp=depth==32?16:depth==16?8:4,stride=w*bpp+16;
  std::vector<unsigned char> source(stride*height),dest(stride*height,0xCD);
  for(int y=0;y<height;++y)for(int x=0;x<w;++x){
   if(depth==32){auto& p=reinterpret_cast<PF_PixelFloat*>(source.data()+y*stride)[x];p.alpha=1;p.red=x/95.f;p.green=y/95.f;p.blue=.5f;}
   else if(depth==16){auto& p=reinterpret_cast<PF_Pixel16*>(source.data()+y*stride)[x];p.alpha=32768;p.red=(x*193+y*107)%32769;p.green=64;p.blue=12345;}
   else{auto& p=reinterpret_cast<PF_Pixel8*>(source.data()+y*stride)[x];p.alpha=255;p.red=x*2;p.green=y*2;p.blue=128;}}
  h.src.width=w;h.src.height=height;h.src.rowbytes=stride;h.src.data=reinterpret_cast<PF_PixelPtr>(source.data());h.dst=h.src;h.dst.data=reinterpret_cast<PF_PixelPtr>(dest.data());
  in.width=w;in.height=height;in.current_time=12;in.time_scale=24;in.time_step=1;
  PF_PreRenderCallbacks pcb{};pcb.checkout_layer=layer;PF_PreRenderInput pi{};pi.bitdepth=depth;pi.output_request.rect={0,0,w,height};PF_PreRenderOutput po{};PF_PreRenderExtra pe{&pi,&po,&pcb};
  h.p[fylm::index("effect.enabled")].u.bd.value=0;
  require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"pre bypass");
  leases("pre bypass leaked parameter leases");
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
  // A rejected frame must still return every parameter it had already checked out.
  h.p[fylm::index("param.threshold")].u.sd.value=9999;
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==PF_Err_BAD_CALLBACK_PARAM,"range rejection");
  require(h.checkins>0&&h.checkins<=fylm::Count-1,"rejected pre leaked parameter leases");h.checkins=0;
  h.p[fylm::index("param.threshold")].u.sd.value=128;
#endif
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"pre bypass restore");
  leases("pre bypass restore leaked parameter leases");
  std::unique_ptr<Frame> frame(static_cast<Frame*>(po.pre_render_data));
  PF_SmartRenderCallbacks cb{pixels,checkin,output};PF_SmartRenderInput si{};si.bitdepth=depth;si.pre_render_data=frame.get();PF_SmartRenderExtra se{&si,&cb};
  require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"bypass");
  for(int y=0;y<height;++y){require(std::memcmp(source.data()+y*stride,dest.data()+y*stride,w*bpp)==0,"bypass channels");for(int x=w*bpp;x<stride;++x)require(dest[y*stride+x]==0xCD,"padding");}
  require(h.checked==0,"checkout leak");
  h.cancel=true;require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==PF_Interrupt_CANCEL,"real cancellation lost");h.cancel=false;
  h.p[fylm::index("effect.enabled")].u.bd.value=1;
#if defined(FYLM_SHADERGLASS)
  h.p[fylm::index("sg.preset")].u.pd.value=depth==16?3:1;
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"ShaderGlass pre");leases("ShaderGlass pre leaked parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();
  require(frame->preset!=nullptr,"ShaderGlass loaded preset snapshot");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"ShaderGlass render");require(h.checked==0,"ShaderGlass checkin");
  if(depth==16)require(frame->preset->name=="crt-easymode.slangp","embedded imported preset not applied");
  auto previousImported=sgRead(&in,h.p[fylm::index("sg.data")].u.arb_d.value);
  for(int choice=4;choice<=fylm::sg::LibraryMenuCount;choice++){
   h.p[2].u.pd.value=choice;po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"library selection pre");leases("library parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();
   require(frame->preset&&frame->preset->id==fylm::sg::builtinPreset(choice)->id,"library entry snapshot mismatch");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"library AE adapter render");require(h.checked==0,"library image lease");
  }
  require(sgRead(&in,h.p[fylm::index("sg.data")].u.arb_d.value)==previousImported,"builtin selection overwrites imported payload");
  h.p[2].u.pd.value=31;h.p[18].u.bd.value=1;h.p[19].u.fs_d.value=2;h.p[20].u.fs_d.value=1;h.p[21].u.sd.value=3;h.p[30].u.bd.value=1;h.p[22].u.fs_d.value=83;
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"SG time and expanded control pre");leases("SG dynamic parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(frame->settings.frameIndex==51&&std::abs(frame->settings.timeSeconds-2.125f)<1e-6&&frame->settings.extendedSlots&&frame->settings.values[8]==83,"AE dynamic clock orI-P binding");
  h.p[18].u.bd.value=0;po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"SG frozen pre");leases("SG frozen parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(frame->settings.frameIndex==27,"frozen phase not preserved");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"SG freeze render");auto frozenImage=dest;in.current_time=48;po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"SG later frozen pre");leases("SG later frozen parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0&&frozenImage==dest,"AE freeze depends on timeline time");in.current_time=12;
  h.p[18].u.bd.value=1;h.p[19].u.fs_d.value=1;h.p[20].u.fs_d.value=0;h.p[21].u.sd.value=0;h.p[30].u.bd.value=0;h.p[22].u.fs_d.value=50;

  PF_ParamDef* paramPointers[fylm::Count];for(int i=0;i<fylm::Count;i++)paramPointers[i]=&h.p[i];PF_UserChangedParamExtra defaultEvent{};defaultEvent.param_index=fylm::index("sg.defaults");auto oldHeight=h.p[3].u.sd.value;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,paramPointers,nullptr,&defaultEvent)!=0&&h.p[3].u.sd.value==oldHeight,"defaults must fail closed without host suites");
#elif defined(FYLM_CRT_ONLY)
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"CRT pre");leases("CRT pre leaked parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));
  require(frame->retro.dither.enabled==0&&frame->retro.retroEnabled&&frame->time==12&&frame->scale==24,"CRT snapshot");
  si.pre_render_data=frame.get();
  auto unavailable=EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se);
  require(unavailable==0,"independent D3D11 must work without an AE GPU suite");
  require(h.checked==0,"GPU failure checkout leak");
#else
  for(int algorithm=1;algorithm<=9;++algorithm){
   h.p[fylm::index("param.algorithm")].u.pd.value=algorithm;po={};
   require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"dither pre");leases("dither pre leaked parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();
   require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"dither render");
   if(algorithm>=7){require(frame->pixelEnabled&&frame->pc98==(algorithm==7)&&frame->pixel.palette.size()==2,"pixel style imported palette snapshot");if(algorithm>7)require(frame->pixel.style==(algorithm==8?pixel_art::Style::GBA:pixel_art::Style::Modern),"style strategy snapshot");auto first=dest;require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0&&first==dest,"pixel style host repeat");
    for(int y=0;y<height;y++)for(int x=0;x<w;x++){auto row=dest.data()+y*stride;if(depth==16){auto q=reinterpret_cast<PF_Pixel16*>(row)[x];require(q.red==32768&&((q.green==0&&q.blue==0)||(q.green==32768&&q.blue==32768))&&q.alpha==32768,"AE16 raw palette invariant");}else if(depth==32){auto q=reinterpret_cast<PF_PixelFloat*>(row)[x];require(q.red==1&&((q.green==0&&q.blue==0)||(q.green==1&&q.blue==1))&&q.alpha==1,"AE32 raw palette invariant");}else{auto q=reinterpret_cast<PF_Pixel8*>(row)[x];require(q.red==255&&((q.green==0&&q.blue==0)||(q.green==255&&q.blue==255))&&q.alpha==255,"AE8 raw palette invariant");}}
   }
   require(h.checked==0,"render checkout leak");
   if(algorithm==6&&depth==8){std::vector<unsigned char> packed(w*height*4),reference(packed.size());
    for(int y=0;y<height;++y)for(int x=0;x<w;++x){auto p=reinterpret_cast<PF_Pixel8*>(source.data()+y*stride)[x];auto i=(y*w+x)*4;packed[i]=p.red;packed[i+1]=p.green;packed[i+2]=p.blue;packed[i+3]=p.alpha;}
    auto result=dither::render({packed.data(),w,height,std::size_t(w)*4,dither::PixelFormat::RGBA8},{reference.data(),w,height,std::size_t(w)*4,dither::PixelFormat::RGBA8},frame->dither);
    require(result.ok,"reference");for(int y=0;y<height;++y)for(int x=0;x<w;++x){auto p=reinterpret_cast<PF_Pixel8*>(dest.data()+y*stride)[x];auto i=(y*w+x)*4;require(p.red==reference[i]&&p.green==reference[i+1]&&p.blue==reference[i+2]&&p.alpha==255,"AE halftone parity");}
   }
  }
  for(int selection:{9,10,11})for(int matrix:{2,3,4}){
   h.p[2].u.pd.value=2;h.p[3].u.pd.value=selection;h.p[73].u.pd.value=matrix;h.p[61].u.pd.value=3;
   require(h.p[62].param_type==PF_Param_COLOR,"native eyedropper colour registration");h.p[62].u.cd.value={255,1,2,3};
   po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"new palette snapshot");leases("new palette leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(frame->dither.palette.size()==4&&frame->dither.orderedMatrix==(matrix==2?2:matrix==3?4:8),"new palette matrix mapping");if(selection==11)require(frame->dither.palette[0].r==1,"eyedropper value snapshot");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0&&h.checked==0,"new palette render");
  }
  h.p[2].u.pd.value=6;h.p[70].u.pd.value=2;h.p[71].u.fs_d.value=20;h.p[72].u.fs_d.value=15;
  po={};require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"CMYK snapshot");leases("CMYK leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(frame->dither.smoothHalftone&&frame->dither.halftoneAngle==20,"CMYK mapping");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"CMYK render");h.p[2].u.pd.value=9;h.p[3].u.pd.value=1;h.p[70].u.pd.value=1;h.p[73].u.pd.value=1;
  h.p[fylm::index("pa.palette")].u.pd.value=3;h.p[fylm::index("pa.reference")].u.fs_d.value=.25;po={};
  require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"reference pre");leases("reference parameter leases");frame.reset(static_cast<Frame*>(po.pre_render_data));si.pre_render_data=frame.get();require(frame->referenceTime==6&&checkoutIds.count(2)&&checkoutIds.count(3),"reference dependency snapshot");require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0&&h.checked==0,"reference render/checkin");h.p[fylm::index("pa.palette")].u.pd.value=2;
#endif
 }
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
 presetHostChecks(h,in,out);
#endif
 std::cout<<fylm::Name<<": registration, isolated UI, snapshots, exact 8/16-bit bypass, padding and cancellation passed (AE cancel="<<PF_Interrupt_CANCEL<<").\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
