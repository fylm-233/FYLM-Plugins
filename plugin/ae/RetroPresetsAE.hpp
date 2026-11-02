#pragma once
#include "fylm/presets.hpp"
#include "AE_GeneralPlug.h"
namespace {
template<class T>struct RetroSuite {
 PF_InData* in;const char* name;int version;const T* suite=nullptr;
 RetroSuite(PF_InData* i,const char* n,int v):in(i),name(n),version(v){if(!in->pica_basicP||in->pica_basicP->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite))||!suite)throw std::runtime_error("retro.error.preset_guard");}
 ~RetroSuite(){in->pica_basicP->ReleaseSuite(name,version);}const T* operator->()const{return suite;}
};
PF_Err retroApplyStyle(PF_InData* in,PF_OutData* out,PF_ParamDef* params[]){
 if(params[fylm::index("retro.mode")]->u.pd.value==1||!params[fylm::index("retro.vhs")]->u.bd.value)throw std::runtime_error("retro.error.preset_mode");
 const auto& recipe=fylm::crt::vhsPreset(params[fylm::index("retro.style")]->u.pd.value);std::map<int,double> values;
 for(auto& [key,value]:recipe.settings){int index=fylm::index(key.c_str());if(index<1)throw std::runtime_error("retro.error.preset_guard");auto spec=fylm::Params[index-1];if(!std::isfinite(value)||value<spec.low||value>spec.high||(spec.kind!=fylm::Kind::Float&&value!=std::round(value)))throw std::runtime_error("retro.error.preset_guard");values[index]=value;}
 RetroSuite<PF_ParamUtilsSuite3> keys(in,kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
 RetroSuite<AEGP_UtilitySuite6> utility(in,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
 RetroSuite<AEGP_PFInterfaceSuite1> bridge(in,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
 RetroSuite<AEGP_EffectSuite4> effects(in,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
 RetroSuite<AEGP_StreamSuite5> streams(in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);
 RetroSuite<AEGP_MemorySuite1> memory(in,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
 static AEGP_PluginID id=0;static bool registered=false;if(!registered){if(utility->AEGP_RegisterWithAEGP(nullptr,"FYLM CRT VHS Presets",&id))throw std::runtime_error("retro.error.preset_guard");registered=true;}
 AEGP_EffectRefH effect=nullptr;if(bridge->AEGP_GetNewEffectForEffect(id,in->effect_ref,&effect)||!effect)throw std::runtime_error("retro.error.preset_guard");
 struct EffectGuard{const AEGP_EffectSuite4* s;AEGP_EffectRefH h;~EffectGuard(){s->AEGP_DisposeEffect(h);}} effectGuard{effects.suite,effect};
 for(auto& [index,value]:values){if(fylm::Params[index-1].kind==fylm::Kind::Popup)continue;PF_KeyIndex count=0;if(keys->PF_GetKeyframeCount(in->effect_ref,index,&count)||count>0)throw std::runtime_error("retro.error.preset_guard");
  AEGP_StreamRefH stream=nullptr;if(streams->AEGP_GetNewEffectStreamByIndex(id,effect,index,&stream)||!stream)throw std::runtime_error("retro.error.preset_guard");
  struct StreamGuard{const AEGP_StreamSuite5* s;AEGP_StreamRefH h;~StreamGuard(){s->AEGP_DisposeStream(h);}} streamGuard{streams.suite,stream};
  A_Boolean enabled=false;if(streams->AEGP_GetExpressionState(id,stream,&enabled)||enabled)throw std::runtime_error("retro.error.preset_guard");
  AEGP_MemHandle text=nullptr;if(streams->AEGP_GetExpression(id,stream,&text))throw std::runtime_error("retro.error.preset_guard");
  if(text){void* data=nullptr;if(memory->AEGP_LockMemHandle(text,&data)||!data){memory->AEGP_FreeMemHandle(text);throw std::runtime_error("retro.error.preset_guard");}bool nonempty=*static_cast<A_UTF16Char*>(data)!=0;memory->AEGP_UnlockMemHandle(text);memory->AEGP_FreeMemHandle(text);if(nonempty)throw std::runtime_error("retro.error.preset_guard");}
 }
 for(auto& [index,value]:values){auto& d=*params[index];switch(fylm::Params[index-1].kind){case fylm::Kind::Popup:d.u.pd.value=A_long(value);break;case fylm::Kind::Check:d.u.bd.value=value!=0;break;case fylm::Kind::Integer:d.u.sd.value=A_long(value);break;default:d.u.fs_d.value=value;break;}d.uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
 out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;return 0;
}
}
