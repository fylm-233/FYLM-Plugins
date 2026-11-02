#pragma once
#include "preset.hpp"
#include "library.hpp"
#include "timing.hpp"
#include "AE_GeneralPlug.h"
#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include <mutex>
#include <cstring>
#include <cstdio>
namespace {
std::filesystem::path sgModuleDirectory(){HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&sgModuleDirectory),&module);wchar_t path[32768]{};GetModuleFileNameW(module,path,32768);return std::filesystem::path(path).parent_path();}
std::vector<unsigned char> sgRead(PF_InData* in_data,PF_ArbitraryH handle){
 if(!handle)return {};auto size=PF_GET_HANDLE_SIZE(handle);if(size<1||size>32*1024*1024)throw std::runtime_error("Invalid preset data size");
 auto data=static_cast<unsigned char*>(PF_LOCK_HANDLE(handle));if(!data)throw std::bad_alloc();std::vector<unsigned char> result;try{result.assign(data,data+size);}catch(...){PF_UNLOCK_HANDLE(handle);throw;}PF_UNLOCK_HANDLE(handle);if(result.size()==1&&result[0]==0)result.clear();return result;
}
PF_ArbitraryH sgMake(PF_InData* in_data,const std::vector<unsigned char>& bytes){auto handle=PF_NEW_HANDLE(std::max<std::size_t>(1,bytes.size()));if(!handle)throw std::bad_alloc();auto data=PF_LOCK_HANDLE(handle);if(!data){PF_DISPOSE_HANDLE(handle);throw std::bad_alloc();}if(bytes.empty())std::memset(data,0,1);else std::memcpy(data,bytes.data(),bytes.size());PF_UNLOCK_HANDLE(handle);return handle;}
PF_Err sgArbitrary(PF_InData* in_data,PF_ArbParamsExtra* e){
 auto copy=[&](PF_ArbitraryH h,PF_ArbitraryH* target){*target=sgMake(in_data,sgRead(in_data,h));};
 switch(e->which_function){
 case PF_Arbitrary_NEW_FUNC:*e->u.new_func_params.arbPH=sgMake(in_data,{});break;
 case PF_Arbitrary_DISPOSE_FUNC:if(e->u.dispose_func_params.arbH)PF_DISPOSE_HANDLE(e->u.dispose_func_params.arbH);break;
 case PF_Arbitrary_COPY_FUNC:copy(e->u.copy_func_params.src_arbH,e->u.copy_func_params.dst_arbPH);break;
 case PF_Arbitrary_FLAT_SIZE_FUNC:*e->u.flat_size_func_params.flat_data_sizePLu=A_u_long(std::max<std::size_t>(1,sgRead(in_data,e->u.flat_size_func_params.arbH).size()));break;
 case PF_Arbitrary_FLATTEN_FUNC:{auto bytes=sgRead(in_data,e->u.flatten_func_params.arbH);if(e->u.flatten_func_params.buf_sizeLu<std::max<std::size_t>(1,bytes.size()))return PF_Err_BAD_CALLBACK_PARAM;if(bytes.empty())std::memset(e->u.flatten_func_params.flat_dataPV,0,1);else std::memcpy(e->u.flatten_func_params.flat_dataPV,bytes.data(),bytes.size());break;}
 case PF_Arbitrary_UNFLATTEN_FUNC:{auto& p=e->u.unflatten_func_params;if(p.buf_sizeLu>32*1024*1024)return PF_Err_BAD_CALLBACK_PARAM;auto b=static_cast<const unsigned char*>(p.flat_dataPV);*p.arbPH=sgMake(in_data,{b,b+p.buf_sizeLu});break;}
 case PF_Arbitrary_INTERP_FUNC:copy(e->u.interp_func_params.left_arbH,e->u.interp_func_params.interpPH);break;
 case PF_Arbitrary_COMPARE_FUNC:*e->u.compare_func_params.compareP=sgRead(in_data,e->u.compare_func_params.a_arbH)==sgRead(in_data,e->u.compare_func_params.b_arbH)?PF_ArbCompare_EQUAL:PF_ArbCompare_NOT_EQUAL;break;
 case PF_Arbitrary_PRINT_SIZE_FUNC:*e->u.print_size_func_params.print_sizePLu=32;break;
 case PF_Arbitrary_PRINT_FUNC:std::snprintf(e->u.print_func_params.print_bufferPC,e->u.print_func_params.print_sizeLu,"FYLM compiled shader preset");break;
 case PF_Arbitrary_SCAN_FUNC:*e->u.scan_func_params.arbPH=sgMake(in_data,{});break;
 default:return PF_Err_BAD_CALLBACK_PARAM;
 }return 0;
}
std::shared_ptr<const fylm::sg::Preset> sgPreset(int choice,const std::vector<unsigned char>& blob){
 if(choice!=3)return fylm::sg::builtinPreset(choice);
 static std::mutex mutex;struct Entry{int choice;std::vector<unsigned char> blob;std::shared_ptr<const fylm::sg::Preset> preset;std::size_t cost;};static std::vector<Entry> cache;static std::size_t retained=0;
 std::lock_guard<std::mutex> lock(mutex);for(auto& e:cache)if(e.choice==choice&&e.blob==blob)return e.preset;
 auto p=fylm::sg::loadBytes(blob);
 fylm::sg::requireRuntimeSupport(*p);
 std::size_t cost=blob.size();for(auto& resource:p->resources)cost+=resource.encoded.size()+resource.rgba.size();constexpr std::size_t limit=128ull*1024*1024;
 while(!cache.empty()&&(cache.size()>=8||retained+cost>limit)){retained-=cache.front().cost;cache.erase(cache.begin());}
 if(cost<=limit){cache.push_back({choice,blob,p,cost});retained+=cost;}return p;
}
template<class T>struct SGSuite {
 PF_InData* in;const char* name;int version;const T* suite=nullptr;
 SGSuite(PF_InData* i,const char* n,int v):in(i),name(n),version(v){if(!in->pica_basicP||in->pica_basicP->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite))||!suite)throw std::runtime_error("sg.error.preset_guard");}
 ~SGSuite(){in->pica_basicP->ReleaseSuite(name,version);}const T* operator->()const{return suite;}
};
void sgGuardParameters(PF_InData* in,const std::map<int,double>& values){
 SGSuite<PF_ParamUtilsSuite3> keys(in,kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
 SGSuite<AEGP_UtilitySuite6> utility(in,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
 SGSuite<AEGP_PFInterfaceSuite1> bridge(in,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
 SGSuite<AEGP_EffectSuite4> effects(in,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
 SGSuite<AEGP_StreamSuite5> streams(in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);
 SGSuite<AEGP_MemorySuite1> memory(in,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
 static AEGP_PluginID id=0;static bool registered=false;if(!registered){if(utility->AEGP_RegisterWithAEGP(nullptr,"FYLM ShaderGlass Presets",&id))throw std::runtime_error("sg.error.preset_guard");registered=true;}
 AEGP_EffectRefH effect=nullptr;if(bridge->AEGP_GetNewEffectForEffect(id,in->effect_ref,&effect)||!effect)throw std::runtime_error("sg.error.preset_guard");
 struct EffectGuard{const AEGP_EffectSuite4* s;AEGP_EffectRefH h;~EffectGuard(){s->AEGP_DisposeEffect(h);}} effectGuard{effects.suite,effect};
 for(auto& [index,value]:values){if(index==13)continue;PF_KeyIndex count=0;if(keys->PF_GetKeyframeCount(in->effect_ref,index,&count)||count>0)throw std::runtime_error("sg.error.preset_guard");
  AEGP_StreamRefH stream=nullptr;if(streams->AEGP_GetNewEffectStreamByIndex(id,effect,index,&stream)||!stream)throw std::runtime_error("sg.error.preset_guard");
  struct StreamGuard{const AEGP_StreamSuite5* s;AEGP_StreamRefH h;~StreamGuard(){s->AEGP_DisposeStream(h);}} streamGuard{streams.suite,stream};
  A_Boolean enabled=false;if(streams->AEGP_GetExpressionState(id,stream,&enabled)||enabled)throw std::runtime_error("sg.error.preset_guard");
  AEGP_MemHandle text=nullptr;if(streams->AEGP_GetExpression(id,stream,&text))throw std::runtime_error("sg.error.preset_guard");
  if(text){void* data=nullptr;if(memory->AEGP_LockMemHandle(text,&data)||!data){memory->AEGP_FreeMemHandle(text);throw std::runtime_error("sg.error.preset_guard");}bool nonempty=*static_cast<A_UTF16Char*>(data)!=0;memory->AEGP_UnlockMemHandle(text);memory->AEGP_FreeMemHandle(text);if(nonempty)throw std::runtime_error("sg.error.preset_guard");}
 }
}
PF_Err sgDefaults(PF_InData* in,PF_OutData* out,PF_ParamDef* params[]){
 int selection=params[fylm::index("sg.preset")]->u.pd.value;
 auto p=sgPreset(selection,selection==3?sgRead(in,params[fylm::index("sg.data")]->u.arb_d.value):std::vector<unsigned char>{});
 std::map<int,double> values;
 for(int slot=0;slot<16;++slot)if(auto c=fylm::sg::slotControl(*p,slot))values[slot<8?5+slot:22+slot-8]=c->high>c->low?std::clamp(100.*(c->value-c->low)/(c->high-c->low),0.,100.):0.;
 values[fylm::index("sg.strength")]=100;values[fylm::index("sg.extended_slots")]=0;
 values[3]=fylm::sg::recommendedHeight(selection,240,true);values[4]=0;values[fylm::index("sg.follow_preset")]=1;
 sgGuardParameters(in,values);
 // All host validation completes before committing any parameter values.
 for(auto& [index,value]:values){auto& d=*params[index];switch(fylm::Params[index-1].kind){case fylm::Kind::Check:d.u.bd.value=value!=0;break;case fylm::Kind::Integer:d.u.sd.value=A_long(value);break;default:d.u.fs_d.value=value;break;}d.uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
 out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;return 0;
}
PF_Err sgCommitPreset(PF_InData* in_data,PF_OutData* out,PF_ParamDef* params[],const std::vector<unsigned char>& bytes){
 auto preset=fylm::sg::loadBytes(bytes);fylm::sg::requireRuntimeSupport(*preset);std::map<int,double> values{{2,3},{13,0},{4,0},{30,0}};for(int slot=0;slot<16;slot++)if(auto u=fylm::sg::slotControl(*preset,slot))values[slot<8?5+slot:22+slot-8]=u->high>u->low?std::clamp(100.*(u->value-u->low)/(u->high-u->low),0.,100.):0.;sgGuardParameters(in_data,values);
 auto handle=sgMake(in_data,bytes);auto old=params[13]->u.arb_d.value;params[13]->u.arb_d.value=handle;params[13]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;if(old)PF_DISPOSE_HANDLE(old);
 params[2]->u.pd.value=3;params[4]->u.bd.value=params[30]->u.bd.value=0;for(int i:{2,4,30})params[i]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
 for(auto& [index,value]:values)if(fylm::Params[index-1].kind==fylm::Kind::Float){params[index]->u.fs_d.value=value;params[index]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
 out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;return 0;
}
PF_Err sgStartImport(PF_InData*,const std::filesystem::path&,const std::filesystem::path&);
PF_Err sgBrowse(PF_InData* in_data,PF_OutData* out,PF_ParamDef* params[]){
 wchar_t path[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();dialog.lpstrFile=path;dialog.nMaxFile=32768;dialog.lpstrFilter=L"FYLM compiled preset (*.fylmsg)\0*.fylmsg\0\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&dialog))return 0;
 std::filesystem::path file(path);if(std::filesystem::file_size(file)>32*1024*1024)throw std::runtime_error("sg.error.size");
 auto result=sgStartImport(in_data,file,{});out->out_flags|=PF_OutFlag_REFRESH_UI;return result;
}
}
#include "ShaderGlassImportAE.hpp"
#include "ShaderGlassEditorAE.hpp"
