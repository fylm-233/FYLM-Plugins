#ifdef FYLM_SHADERGLASS
static Host* sgAsyncHost=nullptr;
static PF_UtilCallbacks* sgAsyncUtils=nullptr;
static int sgAsyncReject=-1,sgAsyncKeys=-1,sgAsyncUndo=0,sgAsyncLeases=0;
static bool sgAsyncMissing=false;
void sgHostImport(Host& h,PF_InData& in,PF_OutData& out){
 sgAsyncHost=&h;sgAsyncUtils=in.utils;
 auto clone=[](PF_Handle handle){auto size=sgAsyncUtils->host_get_handle_size(handle);auto copy=sgAsyncUtils->host_new_handle(size);std::memcpy(*copy,*handle,size);return copy;};
 static decltype(clone)* copyHandle;copyHandle=&clone;
 AEGP_StreamSuite5 streams{};
 streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,PF_ParamIndex i,AEGP_StreamRefH* s)->A_Err{*s=reinterpret_cast<AEGP_StreamRefH>(std::uintptr_t(i));return 0;};
 streams.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{return 0;};
 streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH s,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* v)->A_Err{if(sgAsyncMissing)return 1;int i=int(reinterpret_cast<std::uintptr_t>(s));v->streamH=s;if(i==13)v->val.arbH=reinterpret_cast<AEGP_ArbBlockVal>((*copyHandle)(sgAsyncHost->p[i].u.arb_d.value));else v->val.one_d=i==2?sgAsyncHost->p[i].u.pd.value:sgAsyncHost->p[i].u.bd.value;return 0;};
 streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2* v)->A_Err{if(int(reinterpret_cast<std::uintptr_t>(v->streamH))==13&&v->val.arbH)sgAsyncUtils->host_dispose_handle(reinterpret_cast<PF_Handle>(v->val.arbH));return 0;};
 streams.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH s,AEGP_StreamValue2* v)->A_Err{int i=int(reinterpret_cast<std::uintptr_t>(s));if(i==sgAsyncReject){sgAsyncReject=-1;return 1;}auto& p=sgAsyncHost->p[i];if(i==13){auto copy=(*copyHandle)(reinterpret_cast<PF_Handle>(v->val.arbH));sgAsyncUtils->host_dispose_handle(p.u.arb_d.value);p.u.arb_d.value=copy;}else if(i==2)p.u.pd.value=A_long(v->val.one_d);else p.u.bd.value=v->val.one_d!=0;return 0;};
 streams.AEGP_GetExpressionState=[](AEGP_PluginID,AEGP_StreamRefH,A_Boolean* enabled)->A_Err{*enabled=false;return 0;};streams.AEGP_GetExpression=[](AEGP_PluginID,AEGP_StreamRefH,AEGP_MemHandle* h)->A_Err{*h=nullptr;return 0;};
 AEGP_KeyframeSuite5 keys{};keys.AEGP_GetStreamNumKFs=[](AEGP_StreamRefH s,A_long* count)->A_Err{*count=int(reinterpret_cast<std::uintptr_t>(s))==sgAsyncKeys?1:0;return 0;};
 AEGP_UtilitySuite6 utility{};utility.AEGP_RegisterWithAEGP=[](AEGP_GlobalRefcon,const char*,AEGP_PluginID* id)->A_Err{*id=29;return 0;};utility.AEGP_CauseIdleRoutinesToBeCalled=[]()->A_Err{return 0;};utility.AEGP_StartUndoGroup=[](const char*)->A_Err{sgAsyncUndo++;return 0;};utility.AEGP_EndUndoGroup=[]()->A_Err{sgAsyncUndo--;return 0;};
 AEGP_RegisterSuite5 hooks{};hooks.AEGP_RegisterIdleHook=[](AEGP_PluginID,AEGP_IdleHook,AEGP_IdleRefcon)->A_Err{return 0;};hooks.AEGP_RegisterDeathHook=[](AEGP_PluginID,AEGP_DeathHook,AEGP_DeathRefcon)->A_Err{return 0;};
 AEGP_PFInterfaceSuite1 bridge{};bridge.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr p,AEGP_EffectRefH* h)->A_Err{*h=reinterpret_cast<AEGP_EffectRefH>(p);return 0;};
 AEGP_EffectSuite4 effects{};effects.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{return 0;};AEGP_MemorySuite1 memory{};
 PF_HandleSuite1 handles{};handles.host_new_handle=[](A_HandleSize n){return sgAsyncUtils->host_new_handle(n);};handles.host_get_handle_size=[](PF_Handle h)->A_HandleSize{return sgAsyncUtils->host_get_handle_size(h);};handles.host_lock_handle=[](PF_Handle h){return sgAsyncUtils->host_lock_handle(h);};handles.host_unlock_handle=[](PF_Handle h){sgAsyncUtils->host_unlock_handle(h);};handles.host_dispose_handle=[](PF_Handle h){sgAsyncUtils->host_dispose_handle(h);};
 struct Suites{const void* streams,*keys,*utility,*hooks,*bridge,*effects,*memory,*handles;};static Suites suites;suites={&streams,&keys,&utility,&hooks,&bridge,&effects,&memory,&handles};
 SPBasicSuite basic{};basic.AcquireSuite=[](const char* n,int32,const void** s)->SPErr{*s=nullptr;if(!std::strcmp(n,kAEGPStreamSuite))*s=suites.streams;else if(!std::strcmp(n,kAEGPKeyframeSuite))*s=suites.keys;else if(!std::strcmp(n,kAEGPUtilitySuite))*s=suites.utility;else if(!std::strcmp(n,kAEGPRegisterSuite))*s=suites.hooks;else if(!std::strcmp(n,kAEGPPFInterfaceSuite))*s=suites.bridge;else if(!std::strcmp(n,kAEGPEffectSuite))*s=suites.effects;else if(!std::strcmp(n,kAEGPMemorySuite))*s=suites.memory;else if(!std::strcmp(n,kPFHandleSuite))*s=suites.handles;if(!*s)return -1;sgAsyncLeases++;return 0;};basic.ReleaseSuite=[](const char*,int32)->SPErr{sgAsyncLeases--;return 0;};
 auto prior=in.pica_basicP;in.pica_basicP=&basic;
 auto root=std::filesystem::path(SG_TEST_SOURCE_DIR),helper=root/L"build/modules/shaderglass/Release/FYLM-sgtool.exe",source=root/L"assets/ShaderGlass/crt/crt-geom.slangp";
 auto poll=[](){auto deadline=GetTickCount64()+15000;while(sgImportTask&&GetTickCount64()<deadline){A_long sleep=0;sgImportIdle(nullptr,nullptr,&sleep);Sleep(5);}require(!sgImportTask,"import worker did not complete");};
 auto before=sgRead(&in,h.p[13].u.arb_d.value);h.p[2].u.pd.value=31;h.p[4].u.bd.value=h.p[30].u.bd.value=1;
 sgStartImport(&in,source,helper);require(sgImportTask&&h.p[2].u.pd.value==31&&sgRead(&in,h.p[13].u.arb_d.value)==before,"async start mutated target");poll();require(sgImportStatus=="sg.import_done"&&h.p[2].u.pd.value==3&&!h.p[4].u.bd.value&&!h.p[30].u.bd.value&&sgAsyncUndo==0,"background commit/undo transaction failed");
 auto loaded=sgRead(&in,h.p[13].u.arb_d.value);require(!loaded.empty()&&fylm::sg::loadBytes(loaded)->passes.size()==1,"background preset not embedded");
 for(int scenario=0;scenario<4;scenario++){
  h.p[2].u.pd.value=31;sgStartImport(&in,source,helper);
  if(scenario==0){sgImportTask.reset();require(sgRead(&in,h.p[13].u.arb_d.value)==loaded,"cancel mutated preset");continue;}
  if(scenario==1)h.p[2].u.pd.value=32;
  if(scenario==2)sgAsyncReject=2;
  if(scenario==3)sgAsyncMissing=true;
  poll();sgAsyncMissing=false;
  require(h.p[2].u.pd.value==(scenario==1?32:31)&&sgRead(&in,h.p[13].u.arb_d.value)==loaded&&sgAsyncUndo==0,"stale/deleted target or commit failure lost previous values");
 }
 sgAsyncKeys=4;bool rejected=false;try{sgStartImport(&in,source,helper);}catch(...){rejected=true;}sgAsyncKeys=-1;require(rejected&&!sgImportTask&&sgAsyncLeases==0,"animated import target accepted or suite leaked");
 in.utils->host_dispose_handle(h.p[13].u.arb_d.value);h.p[13].u.arb_d.value=sgMake(&in,before);
 h.p[2].u.pd.value=1;h.p[4].u.bd.value=h.p[30].u.bd.value=0;in.pica_basicP=prior;sgImportBasic=nullptr;sgImportId=0;
}
#endif
