#ifdef FYLM_SHADERGLASS
static int sgTestKeys=-1,sgTestExpr=-1,sgTestLeases=0;
static bool sgTestEnabled=false,sgTestFailure=false;
static PF_ParamUIFlags sgUiFlags[fylm::Count]{};
static int sgUiRejectIndex=-1;
void sgHostDefaults(Host& h,PF_InData& in,PF_OutData& out){
 PF_ParamUtilsSuite3 keys{};keys.PF_GetKeyframeCount=[](PF_ProgPtr,PF_ParamIndex index,PF_KeyIndex* count)->PF_Err{*count=index==sgTestKeys?1:0;return 0;};keys.PF_UpdateParamUI=[](PF_ProgPtr,PF_ParamIndex index,const PF_ParamDef* d)->PF_Err{if(index==sgUiRejectIndex)return PF_Err_BAD_CALLBACK_PARAM;sgUiFlags[index]=d->ui_flags;return 0;};
 AEGP_UtilitySuite6 utility{};utility.AEGP_RegisterWithAEGP=[](AEGP_GlobalRefcon,const A_char*,AEGP_PluginID* id)->A_Err{*id=19;return 0;};
 AEGP_PFInterfaceSuite1 bridge{};bridge.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr effect,AEGP_EffectRefH* handle)->A_Err{*handle=reinterpret_cast<AEGP_EffectRefH>(effect);return 0;};
 AEGP_EffectSuite4 effects{};effects.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{return 0;};
 AEGP_StreamSuite5 streams{};streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,PF_ParamIndex index,AEGP_StreamRefH* stream)->A_Err{*stream=reinterpret_cast<AEGP_StreamRefH>(std::uintptr_t(index));return 0;};streams.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{return 0;};
 streams.AEGP_GetExpressionState=[](AEGP_PluginID,AEGP_StreamRefH stream,A_Boolean* enabled)->A_Err{*enabled=int(reinterpret_cast<std::uintptr_t>(stream))==sgTestExpr&&sgTestEnabled;return 0;};
 streams.AEGP_GetExpression=[](AEGP_PluginID,AEGP_StreamRefH stream,AEGP_MemHandle* text)->A_Err{*text=nullptr;if(int(reinterpret_cast<std::uintptr_t>(stream))==sgTestExpr)*text=reinterpret_cast<AEGP_MemHandle>(new A_UTF16Char[2]{65,0});return 0;};
 AEGP_MemorySuite1 memory{};memory.AEGP_LockMemHandle=[](AEGP_MemHandle handle,void** data)->A_Err{*data=reinterpret_cast<void*>(handle);return 0;};memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle)->A_Err{return 0;};memory.AEGP_FreeMemHandle=[](AEGP_MemHandle handle)->A_Err{delete[] reinterpret_cast<A_UTF16Char*>(handle);return 0;};
 struct Suites{const void* keys;const void* utility;const void* bridge;const void* effects;const void* streams;const void* memory;};static Suites suites;suites={&keys,&utility,&bridge,&effects,&streams,&memory};
 SPBasicSuite basic{};basic.AcquireSuite=[](const char* name,int32,const void** suite)->SPErr{*suite=nullptr;if(std::strcmp(name,kPFParamUtilsSuite)==0)*suite=suites.keys;else if(std::strcmp(name,kAEGPUtilitySuite)==0)*suite=suites.utility;else if(std::strcmp(name,kAEGPPFInterfaceSuite)==0)*suite=suites.bridge;else if(std::strcmp(name,kAEGPEffectSuite)==0)*suite=suites.effects;else if(std::strcmp(name,kAEGPStreamSuite)==0&&!sgTestFailure)*suite=suites.streams;else if(std::strcmp(name,kAEGPMemorySuite)==0)*suite=suites.memory;if(!*suite)return -1;sgTestLeases++;return 0;};basic.ReleaseSuite=[](const char*,int32)->SPErr{sgTestLeases--;return 0;};
 auto prior=in.pica_basicP;in.pica_basicP=&basic;PF_ParamDef* params[fylm::Count];for(int i=0;i<fylm::Count;i++)params[i]=&h.p[i];PF_UserChangedParamExtra event{};event.param_index=15;
 for(const char* key:{"sg.virtual_height","sg.use_slots","sg.extended_slots","sg.follow_preset","sg.animate"})require(h.p[fylm::index(key)].flags&PF_ParamFlag_SUPERVISE,"dependent SG checkbox missing supervised callback");
 h.p[2].u.pd.value=31;h.p[4].u.bd.value=1;h.p[30].u.bd.value=0;event.param_index=30;
 require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"extra bank off UI refresh");for(int i=22;i<=29;i++)require(sgUiFlags[i]&PF_PUI_DISABLED,"disabled extra bank unexpectedly active");
 require(h.p[4].u.bd.value,"turning extra off disabled A-H master");
 h.p[4].u.bd.value=0;h.p[4].uu.change_flags=0;h.p[30].u.bd.value=1;
 require(EffectMain(PF_Cmd_UPDATE_PARAMS_UI,&in,&out,params,nullptr,nullptr)==0,"saved UI state refresh");
 require(!h.p[4].u.bd.value&&!h.p[4].uu.change_flags,"UI refresh auto-enabled saved master-off state");
 out.out_flags=0;
 require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"one-action extra enable");
 require(h.p[4].u.bd.value&&(h.p[4].uu.change_flags&PF_ChangeFlag_CHANGED_VALUE),"extra enable did not commit master in undo transaction");
 require((out.out_flags&PF_OutFlag_FORCE_RERENDER)&&(out.out_flags&PF_OutFlag_REFRESH_UI),"extra enable did not invalidate render and UI");
 for(int i=5;i<=12;i++)require(!(sgUiFlags[i]&PF_PUI_DISABLED),"extra enable did not activate A-H");
 for(int i=22;i<=29;i++)require(!(sgUiFlags[i]&PF_PUI_DISABLED),"extra enable did not activate I-P");
 h.p[30].u.bd.value=1;out.out_flags=0;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"extra bank on UI refresh");require(out.out_flags&PF_OutFlag_REFRESH_UI,"supervised checkbox does not request redraw");for(int i=22;i<=29;i++)require(!(sgUiFlags[i]&PF_PUI_DISABLED),"I-P stays disabled after enabling");
 for(int rejected:{14,15,17}){
  sgUiRejectIndex=rejected;for(int i=22;i<=29;i++)sgUiFlags[i]=PF_PUI_DISABLED;
  require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==PF_Err_BAD_CALLBACK_PARAM,"earlier cosmetic failure was swallowed");
  for(int i=22;i<=29;i++)require(!(sgUiFlags[i]&PF_PUI_DISABLED),"earlier UI failure prevented I-P updates");
  require(h.p[4].u.bd.value&&h.p[30].u.bd.value&&sgTestLeases==0,"UI failure changed switches or leaked suite");
 }
 sgUiRejectIndex=-1;
 h.p[4].u.bd.value=0;event.param_index=4;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"master off UI refresh");for(int i=22;i<=29;i++)require(sgUiFlags[i]&PF_PUI_DISABLED,"I-P ignores master off");
 h.p[4].u.bd.value=1;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"master on UI refresh");for(int i=22;i<=29;i++)require(!(sgUiFlags[i]&PF_PUI_DISABLED),"I-P stays disabled after master on");
 h.p[2].u.pd.value=1;event.param_index=2;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"legacy UI refresh");for(int i=22;i<=29;i++)require(sgUiFlags[i]&PF_PUI_DISABLED,"legacy preset exposes nonexistent extra controls");
 h.p[2].u.pd.value=31;h.p[16].u.bd.value=1;h.p[3].u.sd.value=180;event.param_index=3;require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"manual input grid event");require(!h.p[16].u.bd.value&&h.p[3].u.sd.value==180&&!(sgUiFlags[3]&PF_PUI_DISABLED),"manual grid remains locked or overridden");
 event.param_index=15;h.p[30].u.bd.value=0;
 h.p[2].u.pd.value=4;h.p[3].u.sd.value=333;h.p[4].u.bd.value=1;h.p[5].u.fs_d.value=99;h.p[16].u.bd.value=0;
 require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)==0,"default restore successful host path");
 require(h.p[2].u.pd.value==4&&h.p[3].u.sd.value==720&&!h.p[4].u.bd.value&&h.p[16].u.bd.value,"default restore scope");require(std::abs(h.p[5].u.fs_d.value-8)<1e-4&&sgTestLeases==0,"default normalized control or suite leak");
 for(int scenario=0;scenario<4;scenario++){
  h.p[3].u.sd.value=333;h.p[4].u.bd.value=1;h.p[5].u.fs_d.value=99;h.p[16].u.bd.value=0;
  sgTestKeys=scenario==0?5:-1;sgTestExpr=scenario==1||scenario==2?5:-1;sgTestEnabled=scenario==1;sgTestFailure=scenario==3;
  require(EffectMain(PF_Cmd_USER_CHANGED_PARAM,&in,&out,params,nullptr,&event)!=0,"default restore guard accepted conflict");
  require(h.p[3].u.sd.value==333&&h.p[4].u.bd.value&&h.p[5].u.fs_d.value==99&&!h.p[16].u.bd.value&&sgTestLeases==0,"default guard changed state or leaked suite");
 }
 sgTestKeys=sgTestExpr=-1;sgTestEnabled=sgTestFailure=false;
 {std::vector<PF_ParamDef> saved(h.p,h.p+fylm::Count);auto savedBlob=sgRead(&in,h.p[13].u.arb_d.value);auto custom=*fylm::sg::builtinPreset(39);auto name=custom.controls.back().name;float value=custom.controls.back().low+(custom.controls.back().high-custom.controls.back().low)*.25f;for(auto& u:custom.controls)if(u.name==name)u.value=value;for(auto& pass:custom.passes)for(auto& u:pass.uniforms)if(u.name==name)u.value=value;custom.slots.resize(16);for(auto& slot:custom.slots)if(slot==name)slot.clear();custom.slots[15]=name;
  auto bytes=fylm::sg::encode(custom);require(sgCommitPreset(&in,&out,params,bytes)==0,"advanced custom copy commit");require(h.p[2].u.pd.value==3&&!h.p[4].u.bd.value&&std::abs(h.p[29].u.fs_d.value-25)<1e-4,"advanced mapping defaults not initialized");auto loaded=fylm::sg::loadBytes(sgRead(&in,h.p[13].u.arb_d.value));require(fylm::sg::slotControl(*loaded,15)->name==name,"advanced slot binding not persisted");h.p[4].u.bd.value=h.p[30].u.bd.value=1;h.p[29].u.fs_d.value=75;auto snapshot=sgCurrentValues(&in,params);require(std::abs(fylm::sg::slotControl(snapshot,15)->value-(custom.controls.back().low+(custom.controls.back().high-custom.controls.back().low)*.75f))<1e-5,"export snapshot ignores actual animated-slot value");
  auto committed=sgRead(&in,h.p[13].u.arb_d.value);sgTestKeys=29;bool rejected=false;try{sgCommitPreset(&in,&out,params,bytes);}catch(...){rejected=true;}sgTestKeys=-1;require(rejected&&sgRead(&in,h.p[13].u.arb_d.value)==committed&&h.p[29].u.fs_d.value==75,"advanced apply overwrote animated control");in.utils->host_dispose_handle(h.p[13].u.arb_d.value);for(int i=1;i<fylm::Count;i++)if(i!=13)h.p[i]=saved[i];h.p[13].u.arb_d.value=sgMake(&in,savedBlob);
 }
 h.p[4].u.bd.value=0;h.p[2].u.pd.value=1;in.pica_basicP=prior;
}
#endif
