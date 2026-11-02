#pragma once
#include "pixel_art/core.hpp"
#include "pixel_art/presets.hpp"
#include "AE_GeneralPlug.h"
#include "json.hpp"
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdio>
namespace {
std::vector<unsigned char> paRead(PF_InData* in_data,PF_ArbitraryH handle){
 if(!handle)return {};auto size=PF_GET_HANDLE_SIZE(handle);if(size<1||size>65536)throw std::runtime_error("Invalid preset data size");
 auto data=static_cast<unsigned char*>(PF_LOCK_HANDLE(handle));if(!data)throw std::bad_alloc();std::vector<unsigned char> result;try{result.assign(data,data+size);}catch(...){PF_UNLOCK_HANDLE(handle);throw;}PF_UNLOCK_HANDLE(handle);if(result.size()==1&&result[0]==0)result.clear();return result;
}
PF_ArbitraryH paMake(PF_InData* in_data,const std::vector<unsigned char>& bytes){auto handle=PF_NEW_HANDLE(std::max<std::size_t>(1,bytes.size()));if(!handle)throw std::bad_alloc();auto data=PF_LOCK_HANDLE(handle);if(!data){PF_DISPOSE_HANDLE(handle);throw std::bad_alloc();}if(bytes.empty())std::memset(data,0,1);else std::memcpy(data,bytes.data(),bytes.size());PF_UNLOCK_HANDLE(handle);return handle;}
PF_Err paArbitrary(PF_InData* in_data,PF_ArbParamsExtra* e){
 auto copy=[&](PF_ArbitraryH h,PF_ArbitraryH* target){*target=paMake(in_data,paRead(in_data,h));};
 switch(e->which_function){
 case PF_Arbitrary_NEW_FUNC:*e->u.new_func_params.arbPH=paMake(in_data,{});break;
 case PF_Arbitrary_DISPOSE_FUNC:if(e->u.dispose_func_params.arbH)PF_DISPOSE_HANDLE(e->u.dispose_func_params.arbH);break;
 case PF_Arbitrary_COPY_FUNC:copy(e->u.copy_func_params.src_arbH,e->u.copy_func_params.dst_arbPH);break;
 case PF_Arbitrary_FLAT_SIZE_FUNC:*e->u.flat_size_func_params.flat_data_sizePLu=A_u_long(std::max<std::size_t>(1,paRead(in_data,e->u.flat_size_func_params.arbH).size()));break;
 case PF_Arbitrary_FLATTEN_FUNC:{auto bytes=paRead(in_data,e->u.flatten_func_params.arbH);if(e->u.flatten_func_params.buf_sizeLu<std::max<std::size_t>(1,bytes.size()))return PF_Err_BAD_CALLBACK_PARAM;if(bytes.empty())std::memset(e->u.flatten_func_params.flat_dataPV,0,1);else std::memcpy(e->u.flatten_func_params.flat_dataPV,bytes.data(),bytes.size());break;}
 case PF_Arbitrary_UNFLATTEN_FUNC:{auto& p=e->u.unflatten_func_params;if(p.buf_sizeLu>65536)return PF_Err_BAD_CALLBACK_PARAM;auto b=static_cast<const unsigned char*>(p.flat_dataPV);*p.arbPH=paMake(in_data,{b,b+p.buf_sizeLu});break;}
 case PF_Arbitrary_INTERP_FUNC:copy(e->u.interp_func_params.left_arbH,e->u.interp_func_params.interpPH);break;
 case PF_Arbitrary_COMPARE_FUNC:*e->u.compare_func_params.compareP=paRead(in_data,e->u.compare_func_params.a_arbH)==paRead(in_data,e->u.compare_func_params.b_arbH)?PF_ArbCompare_EQUAL:PF_ArbCompare_NOT_EQUAL;break;
 case PF_Arbitrary_PRINT_SIZE_FUNC:*e->u.print_size_func_params.print_sizePLu=32;break;
 case PF_Arbitrary_PRINT_FUNC:std::snprintf(e->u.print_func_params.print_bufferPC,e->u.print_func_params.print_sizeLu,"FYLM sRGB palette");break;
 case PF_Arbitrary_SCAN_FUNC:*e->u.scan_func_params.arbPH=paMake(in_data,{});break;
 default:return PF_Err_BAD_CALLBACK_PARAM;
 }return 0;
}

std::vector<pixel_art::Rgb8> paDecode(const std::vector<unsigned char>& bytes){
 try{
 auto j=nlohmann::json::parse(bytes.begin(),bytes.end());if(j.at("schema_version")!=1||j.at("encoding")!="srgb")throw std::runtime_error("palette schema");
 auto colors=j.at("colors");if(!colors.is_array()||colors.size()<2||colors.size()>256)throw std::runtime_error("palette count");std::vector<pixel_art::Rgb8> result;
 for(auto& value:colors){auto s=value.get<std::string>();if(s.size()!=7||s[0]!='#'||s.find_first_not_of("0123456789abcdefABCDEF",1)!=std::string::npos)throw std::runtime_error("palette hex");unsigned n=std::stoul(s.substr(1),nullptr,16);pixel_art::Rgb8 c{static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n)};for(auto v:result)if(v.r==c.r&&v.g==c.g&&v.b==c.b)throw std::runtime_error("duplicate palette color");result.push_back(c);}return result;
 }catch(...){throw std::runtime_error("pa.error.palette");}
}
std::vector<unsigned char> paEncode(const std::vector<pixel_art::Rgb8>& colors){nlohmann::json j={{"schema_version",1},{"name","FYLM Pixel Art palette"},{"encoding","srgb"},{"colors",nlohmann::json::array()}};for(auto c:colors){char hex[8];std::snprintf(hex,sizeof(hex),"#%02X%02X%02X",c.r,c.g,c.b);j["colors"].push_back(hex);}auto s=j.dump(2);return {s.begin(),s.end()};}
template<class T>struct PASuite {
 PF_InData* in;const char* name;int version;const T* suite=nullptr;
 PASuite(PF_InData* i,const char* n,int v):in(i),name(n),version(v){if(!in->pica_basicP||in->pica_basicP->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite))||!suite)throw std::runtime_error("pa.error.preset_guard");}
 ~PASuite(){in->pica_basicP->ReleaseSuite(name,version);}const T* operator->()const{return suite;}
};
AEGP_PluginID paLibraryPluginId(PF_InData* in){
 PASuite<AEGP_UtilitySuite6> utility(in,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);static AEGP_PluginID id=0;static bool registered=false;
 if(!registered){if(utility->AEGP_RegisterWithAEGP(nullptr,"FYLM Pixel Art Presets",&id))throw std::runtime_error("pa.error.preset_guard");registered=true;}return id;
}
int paLibraryMode(int index){return index>=52&&index<=57?7+(index-52)/2:0;}
int paLibraryChooser(int mode){return mode>=7&&mode<=9?52+(mode-7)*2:-1;}
void paLibraryVisibility(PF_InData* in,int mode,PF_ParamDef* params[],bool (*inactive)(int,PF_ParamDef*[])){
 auto id=paLibraryPluginId(in);PASuite<AEGP_PFInterfaceSuite1> bridge(in,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
 PASuite<AEGP_EffectSuite4> effects(in,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);PASuite<AEGP_StreamSuite5> streams(in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);
 PASuite<AEGP_DynamicStreamSuite4> dynamic(in,kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4);
 AEGP_EffectRefH effect=nullptr;if(bridge->AEGP_GetNewEffectForEffect(id,in->effect_ref,&effect)||!effect)throw std::runtime_error("pa.error.preset_guard");
 struct EffectGuard{const AEGP_EffectSuite4* s;AEGP_EffectRefH h;~EffectGuard(){s->AEGP_DisposeEffect(h);}} effectGuard{effects.suite,effect};
 for(int i=1;i<fylm::Count;i++){if(fylm::Params[i-1].kind==fylm::Kind::Blob)continue;AEGP_StreamRefH stream=nullptr;if(streams->AEGP_GetNewEffectStreamByIndex(id,effect,i,&stream)||!stream)throw std::runtime_error("pa.error.preset_guard");
  struct StreamGuard{const AEGP_StreamSuite5* s;AEGP_StreamRefH h;~StreamGuard(){s->AEGP_DisposeStream(h);}} guard{streams.suite,stream};AEGP_DynStreamFlags flags=0;
  if(dynamic->AEGP_GetDynamicStreamFlags(stream,&flags))throw std::runtime_error("pa.error.preset_guard");bool hidden=i>=49&&i<=57?(paLibraryMode(i)==0||paLibraryMode(i)!=mode):inactive(i,params);
  if(bool(flags&AEGP_DynStreamFlag_HIDDEN)!=hidden&&dynamic->AEGP_SetDynamicStreamFlag(stream,AEGP_DynStreamFlag_HIDDEN,FALSE,hidden))throw std::runtime_error("pa.error.preset_guard");
 }
}
void paPresetGuard(PF_InData* in,const std::map<int,double>& values){
 PASuite<PF_ParamUtilsSuite3> params(in,kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
 std::vector<int> varying;for(auto& entry:values){auto kind=fylm::Params[entry.first-1].kind;if(kind==fylm::Kind::Popup)continue;PF_KeyIndex count=0;if(params->PF_GetKeyframeCount(in->effect_ref,entry.first,&count))throw std::runtime_error("pa.error.preset_guard");if(count>0)throw std::runtime_error("pa.error.preset_conflict");varying.push_back(entry.first);}
 auto pluginId=paLibraryPluginId(in);
 PASuite<AEGP_PFInterfaceSuite1> bridge(in,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
 PASuite<AEGP_EffectSuite4> effects(in,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
 PASuite<AEGP_StreamSuite5> streams(in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);
 PASuite<AEGP_MemorySuite1> memory(in,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
 AEGP_EffectRefH effect=nullptr;if(bridge->AEGP_GetNewEffectForEffect(pluginId,in->effect_ref,&effect)||!effect)throw std::runtime_error("pa.error.preset_guard");
 struct EffectGuard{const AEGP_EffectSuite4* suite;AEGP_EffectRefH handle;~EffectGuard(){suite->AEGP_DisposeEffect(handle);}} effectGuard{effects.suite,effect};
 for(int index:varying){AEGP_StreamRefH stream=nullptr;if(streams->AEGP_GetNewEffectStreamByIndex(pluginId,effect,index,&stream)||!stream)throw std::runtime_error("pa.error.preset_guard");struct StreamGuard{const AEGP_StreamSuite5* suite;AEGP_StreamRefH handle;~StreamGuard(){suite->AEGP_DisposeStream(handle);}} streamGuard{streams.suite,stream};
  A_Boolean enabled=false;if(streams->AEGP_GetExpressionState(pluginId,stream,&enabled))throw std::runtime_error("pa.error.preset_guard");if(enabled)throw std::runtime_error("pa.error.preset_conflict");
  AEGP_MemHandle text=nullptr;if(streams->AEGP_GetExpression(pluginId,stream,&text))throw std::runtime_error("pa.error.preset_guard");if(text){void* data=nullptr;auto err=memory->AEGP_LockMemHandle(text,&data);if(err||!data){memory->AEGP_FreeMemHandle(text);throw std::runtime_error("pa.error.preset_guard");}bool nonempty=*static_cast<const A_UTF16Char*>(data)!=0;memory->AEGP_UnlockMemHandle(text);memory->AEGP_FreeMemHandle(text);if(nonempty)throw std::runtime_error("pa.error.preset_conflict");}
 }
}
PF_Err paApplyLibrary(PF_InData* in,PF_OutData* out,PF_ParamDef* params[],int action){
 int mode=params[2]->u.pd.value,chooser=paLibraryChooser(mode);if(chooser<0||action!=chooser+1)throw std::runtime_error("pa.error.preset_mode");
 int choice=params[chooser]->u.pd.value;if(choice<2||choice>fylm::Params[chooser-1].choices)throw std::runtime_error("pa.error.preset_choice");
 int ordinal=(choice-2)/2,variant=(choice-2)%2;const pixel_art::ArtPreset* selected=nullptr;for(auto& entry:pixel_art::artPresets())if(entry.mode==mode&&ordinal--==0){selected=&entry;break;}
 if(!selected||selected->mode!=mode)throw std::runtime_error("pa.error.preset_mode");auto& preset=*selected;auto recipe=pixel_art::artPresetValues(preset,variant,in->width);recipe.erase("param.algorithm");std::map<int,double> values;
 for(auto& entry:recipe){int index=fylm::index(entry.first.c_str());if(index<1)throw std::runtime_error("pa.error.preset_schema");auto spec=fylm::Params[index-1];double value=entry.second;if(!std::isfinite(value)||value<spec.low||value>spec.high||(spec.kind!=fylm::Kind::Float&&value!=std::round(value)))throw std::runtime_error("pa.error.preset_schema");values[index]=value;}
 auto bytes=pixel_art::artPresetPalette(preset,variant,in->width);paDecode(bytes);paPresetGuard(in,values);
 PF_InData* in_data=in;auto handle=paMake(in,bytes);int blob=fylm::index("pa.data");auto previous=params[blob]->u.arb_d.value;
 // No host calls can fail between validation/allocation and this commit.
 for(auto& entry:values){auto& param=*params[entry.first];auto kind=fylm::Params[entry.first-1].kind;if(kind==fylm::Kind::Integer)param.u.sd.value=A_long(entry.second);else if(kind==fylm::Kind::Popup)param.u.pd.value=A_long(entry.second);else if(kind==fylm::Kind::Check)param.u.bd.value=entry.second!=0;else param.u.fs_d.value=entry.second;param.uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
 params[blob]->u.arb_d.value=handle;params[blob]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;if(previous)PF_DISPOSE_HANDLE(previous);
 out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;return 0;
}
std::vector<unsigned char> paExportData(PF_InData* in,PF_ParamDef* params[]){
 int mode=params[2]->u.pd.value,source=params[fylm::index("pa.palette")]->u.pd.value;
 if(source==3)throw std::runtime_error("pa.error.palette");
 return source==2?paRead(in,params[fylm::index("pa.data")]->u.arb_d.value):paEncode(pixel_art::stylePalette(mode==8?pixel_art::Style::GBA:mode==9?pixel_art::Style::Modern:pixel_art::Style::Basic));
}
PF_Err paAction(PF_InData* in,PF_OutData* out,PF_ParamDef* params[],int action,dither::i18n::Locale language){
 PF_InData* in_data=in;
 if(action==fylm::index("pa.preset")){
  int mode=params[2]->u.pd.value;if(mode<7||mode>9)mode=7;
  std::vector<int> affected;
  for(int i=15;i<=26;i++)if(mode==7||i==19||i==20||i>=22)affected.push_back(i);
  if(mode==8)for(int i=31;i<=35;i++)affected.push_back(i);
  if(mode==9)for(int i=36;i<=48;i++)affected.push_back(i);
  if(mode==7){affected.push_back(fylm::index("pa.indexed_colors"));affected.push_back(fylm::index("pa.dac"));}
  const PF_ParamUtilsSuite3* suite=nullptr;if(in->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite)))return PF_Err_BAD_CALLBACK_PARAM;
  bool animated=false;for(int i:affected){PF_KeyIndex keys=0;auto err=suite->PF_GetKeyframeCount(in->effect_ref,i,&keys);if(err||keys>0)animated=true;}in->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
  if(animated){dither::i18n::copyUtf8(out->return_msg,sizeof(out->return_msg),dither::i18n::text(language,"pa.error.animated"));out->out_flags|=PF_OutFlag_DISPLAY_ERROR_MESSAGE;return PF_Err_BAD_CALLBACK_PARAM;}
  for(int i:affected){auto spec=fylm::Params[i-1];if(spec.kind==fylm::Kind::Integer)params[i]->u.sd.value=A_long(spec.value);else if(spec.kind==fylm::Kind::Popup)params[i]->u.pd.value=A_long(spec.value);else if(spec.kind==fylm::Kind::Check)params[i]->u.bd.value=spec.value!=0;else params[i]->u.fs_d.value=spec.value;params[i]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
  params[2]->u.pd.value=mode;params[2]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
 }else{
  bool exporting=action==fylm::index("pa.export");wchar_t path[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();dialog.lpstrFile=path;dialog.nMaxFile=32768;dialog.lpstrFilter=L"sRGB palette JSON\0*.json\0\0";dialog.lpstrDefExt=L"json";dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(exporting?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
  if(exporting){auto bytes=paExportData(in,params);if(!GetSaveFileNameW(&dialog))return 0;std::ofstream file(std::filesystem::path(path),std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!file)return PF_Err_BAD_CALLBACK_PARAM;return 0;}
  if(!GetOpenFileNameW(&dialog))return 0;std::filesystem::path file(path);if(std::filesystem::file_size(file)>65536)return PF_Err_BAD_CALLBACK_PARAM;std::ifstream input(file,std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};paDecode(bytes);
  auto index=fylm::index("pa.data");auto handle=paMake(in,bytes);auto previous=params[index]->u.arb_d.value;params[index]->u.arb_d.value=handle;params[index]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;if(previous)PF_DISPOSE_HANDLE(previous);
  index=fylm::index("pa.palette");params[index]->u.pd.value=2;params[index]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
 }
 out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;return 0;
}
} // namespace
