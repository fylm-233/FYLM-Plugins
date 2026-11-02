#pragma once
#include "../../modules/shaderglass/tool_process.hpp"
#include <array>
namespace {
// All AEGP calls and parameter commits happen in AE's UI/idle thread. Only the
// isolated compiler process runs in the background; no PF_InData is retained.
struct SGImportTask {
 SPBasicSuite* basic=nullptr;AEGP_PluginID id=0;
 std::array<AEGP_StreamRefH,4> streams{};
 std::array<AEGP_StreamValue2,4> before{};int captured=0;
 std::vector<unsigned char> original;
 std::filesystem::path directory,output;
 std::unique_ptr<fylm::sg::ToolProcess> worker;
 ~SGImportTask(){worker.reset();if(basic){try{PF_InData in{};in.pica_basicP=basic;SGSuite<AEGP_StreamSuite5> suite(&in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);for(int i=0;i<captured;i++)suite->AEGP_DisposeStreamValue(&before[i]);for(auto stream:streams)if(stream)suite->AEGP_DisposeStream(stream);}catch(...){}}
  std::error_code error;if(!output.empty())std::filesystem::remove(output,error);if(!directory.empty()){for(auto& file:std::filesystem::directory_iterator(directory,error))if(file.is_regular_file(error)&&file.path().filename().wstring().starts_with(L"compiled.fylmsg.partial-"))std::filesystem::remove(file.path(),error);std::filesystem::remove(directory,error);}
 }
};
std::unique_ptr<SGImportTask> sgImportTask;
std::string sgImportStatus="sg.import_idle";
std::string sgImportError(const std::string& output){if(output.find("Temporal feedback/history")!=std::string::npos)return "sg.error.temporal";if(output.find("Unsupported #pragma format")!=std::string::npos)return "sg.error.format_unsupported";if(output.find("Missing/oversized resource texture")!=std::string::npos)return "sg.error.resource";for(const char* key:{"sg.error.temporal","sg.error.timeout","sg.error.validation","sg.error.hash","sg.error.resource_budget","sg.error.resource","sg.error.format_unsupported","sg.error.schema","sg.error.size","sg.error.memory","sg.error.worker","sg.error.write"})if(output.find(key)!=std::string::npos)return key;return "sg.error.compiler";}
SPBasicSuite* sgImportBasic=nullptr;AEGP_PluginID sgImportId=0;
void sgImportLog(const std::string& text)noexcept{try{wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);std::ofstream out(std::filesystem::path(temp)/L"FYLM-ShaderGlass-import.log",std::ios::app|std::ios::binary);out<<"version="<<FYLM_VERSION_TEXT<<",pid="<<GetCurrentProcessId()<<","<<text<<'\n';}catch(...){}}
std::vector<unsigned char> sgImportBytes(const PF_HandleSuite1* handles,AEGP_ArbBlockVal h){if(!h)return {};auto handle=reinterpret_cast<PF_Handle>(h);auto size=handles->host_get_handle_size(handle);if(size<1||size>32*1024*1024)throw std::runtime_error("sg.error.size");auto data=static_cast<unsigned char*>(handles->host_lock_handle(handle));if(!data)throw std::bad_alloc();std::vector<unsigned char> bytes;try{bytes.assign(data,data+size);}catch(...){handles->host_unlock_handle(handle);throw;}handles->host_unlock_handle(handle);return bytes;}
void sgImportCheck(SGImportTask& task,const AEGP_StreamSuite5* streams,const AEGP_KeyframeSuite5* keys,const AEGP_MemorySuite1* memory){
 for(int i=0;i<4;i++){A_long count=0;if(keys->AEGP_GetStreamNumKFs(task.streams[i],&count)||count>0)throw std::runtime_error("sg.error.preset_guard");if(i==0)continue;
  A_Boolean enabled=false;if(streams->AEGP_GetExpressionState(task.id,task.streams[i],&enabled)||enabled)throw std::runtime_error("sg.error.preset_guard");AEGP_MemHandle text=nullptr;if(streams->AEGP_GetExpression(task.id,task.streams[i],&text))throw std::runtime_error("sg.error.preset_guard");
  if(text){void* data=nullptr;auto error=memory->AEGP_LockMemHandle(text,&data);bool nonempty=!error&&data&&*static_cast<A_UTF16Char*>(data);if(!error)memory->AEGP_UnlockMemHandle(text);memory->AEGP_FreeMemHandle(text);if(error||nonempty)throw std::runtime_error("sg.error.preset_guard");}
 }
}
void sgFinishImport(SGImportTask& task){
 auto preset=fylm::sg::load(task.output);fylm::sg::requireRuntimeSupport(*preset);
 std::ifstream input(task.output,std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};
 PF_InData in{};in.pica_basicP=task.basic;
 SGSuite<AEGP_StreamSuite5> streams(&in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);SGSuite<AEGP_KeyframeSuite5> keys(&in,kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5);SGSuite<AEGP_MemorySuite1> memory(&in,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);SGSuite<PF_HandleSuite1> handles(&in,kPFHandleSuite,kPFHandleSuiteVersion1);SGSuite<AEGP_UtilitySuite6> utility(&in,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
 sgImportCheck(task,streams.suite,keys.suite,memory.suite);
 // Never overwrite an effect which was edited/replaced/deleted during compile.
 A_Time time{0,1};for(int i=0;i<4;i++){AEGP_StreamValue2 now{};if(streams->AEGP_GetNewStreamValue(task.id,task.streams[i],AEGP_LTimeMode_LayerTime,&time,TRUE,&now))throw std::runtime_error("sg.error.import_stale");bool same=false;try{same=i==0?sgImportBytes(handles.suite,now.val.arbH)==task.original:now.val.one_d==task.before[i].val.one_d;}catch(...){streams->AEGP_DisposeStreamValue(&now);throw;}streams->AEGP_DisposeStreamValue(&now);if(!same)throw std::runtime_error("sg.error.import_stale");}
 auto handle=handles->host_new_handle(bytes.size());if(!handle)throw std::bad_alloc();struct HandleGuard{const PF_HandleSuite1* s;PF_Handle h;~HandleGuard(){s->host_dispose_handle(h);}} guard{handles.suite,handle};auto data=handles->host_lock_handle(handle);if(!data)throw std::bad_alloc();std::memcpy(data,bytes.data(),bytes.size());handles->host_unlock_handle(handle);
 if(utility->AEGP_StartUndoGroup("FYLM ShaderGlass Import"))throw std::runtime_error("sg.error.preset_guard");
 struct UndoGuard{const AEGP_UtilitySuite6* s;~UndoGuard(){s->AEGP_EndUndoGroup();}} undo{utility.suite};
 for(int i=0;i<4;i++){AEGP_StreamValue2 value{};value.streamH=task.streams[i];if(i==0)value.val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(handle);else value.val.one_d=i==1?3:0;
  if(streams->AEGP_SetStreamValue(task.id,task.streams[i],&value)){for(int j=i;j>=0;j--)if(streams->AEGP_SetStreamValue(task.id,task.streams[j],&task.before[j]))sgImportLog("rollback-failed,index="+std::to_string(j));throw std::runtime_error("sg.error.import_commit");}
 }
}
A_Err sgImportIdle(AEGP_GlobalRefcon,AEGP_IdleRefcon,A_long* sleep){
 if(!sgImportTask)return 0;if(sleep)*sleep=6;
 try{if(!sgImportTask->worker->poll())return 0;if(sgImportTask->worker->timedOut)throw std::runtime_error("sg.error.timeout");if(sgImportTask->worker->exitCode){sgImportLog(sgImportTask->worker->output);throw std::runtime_error(sgImportError(sgImportTask->worker->output));}sgFinishImport(*sgImportTask);sgImportStatus="sg.import_done";sgImportLog("completed");}
 catch(const std::exception& error){sgImportStatus=error.what();sgImportLog(sgImportStatus);}
 catch(...){sgImportStatus="sg.error.compiler";sgImportLog(sgImportStatus);}
 sgImportTask.reset();return 0;
}
A_Err sgImportDeath(AEGP_GlobalRefcon,AEGP_DeathRefcon){sgImportTask.reset();return 0;}
PF_Err sgStartImport(PF_InData* in,const std::filesystem::path& source,const std::filesystem::path& helper={}){
 if(sgImportTask)throw std::runtime_error("sg.error.import_busy");
 SGSuite<AEGP_UtilitySuite6> utility(in,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
 if(!sgImportId){if(utility->AEGP_RegisterWithAEGP(nullptr,"FYLM ShaderGlass Import",&sgImportId))throw std::runtime_error("sg.error.worker");sgImportBasic=in->pica_basicP;SGSuite<AEGP_RegisterSuite5> hooks(in,kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5);if(hooks->AEGP_RegisterIdleHook(sgImportId,sgImportIdle,nullptr)||hooks->AEGP_RegisterDeathHook(sgImportId,sgImportDeath,nullptr)){sgImportId=0;throw std::runtime_error("sg.error.worker");}}
 auto task=std::make_unique<SGImportTask>();task->basic=in->pica_basicP;task->id=sgImportId;
 SGSuite<AEGP_PFInterfaceSuite1> bridge(in,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);SGSuite<AEGP_EffectSuite4> effects(in,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);SGSuite<AEGP_StreamSuite5> streams(in,kAEGPStreamSuite,kAEGPStreamSuiteVersion5);SGSuite<AEGP_KeyframeSuite5> keys(in,kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5);SGSuite<AEGP_MemorySuite1> memory(in,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);SGSuite<PF_HandleSuite1> handles(in,kPFHandleSuite,kPFHandleSuiteVersion1);
 AEGP_EffectRefH effect=nullptr;if(bridge->AEGP_GetNewEffectForEffect(task->id,in->effect_ref,&effect)||!effect)throw std::runtime_error("sg.error.preset_guard");struct EffectGuard{const AEGP_EffectSuite4* s;AEGP_EffectRefH h;~EffectGuard(){s->AEGP_DisposeEffect(h);}} effectGuard{effects.suite,effect};
 const int indices[]={13,2,4,30};A_Time time{0,1};for(int i=0;i<4;i++){if(streams->AEGP_GetNewEffectStreamByIndex(task->id,effect,indices[i],&task->streams[i])||!task->streams[i])throw std::runtime_error("sg.error.preset_guard");if(streams->AEGP_GetNewStreamValue(task->id,task->streams[i],AEGP_LTimeMode_LayerTime,&time,TRUE,&task->before[i]))throw std::runtime_error("sg.error.preset_guard");task->captured++;}
 sgImportCheck(*task,streams.suite,keys.suite,memory.suite);task->original=sgImportBytes(handles.suite,task->before[0].val.arbH);
 wchar_t temp[MAX_PATH]{};if(!GetTempPathW(MAX_PATH,temp))throw std::runtime_error("sg.error.write");task->directory=std::filesystem::path(temp)/(L"FYLM-SG-import-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));if(!std::filesystem::create_directory(task->directory))throw std::runtime_error("sg.error.write");task->output=task->directory/L"compiled.fylmsg";
 task->worker=std::make_unique<fylm::sg::ToolProcess>((helper.empty()?sgModuleDirectory()/L"FYLM-sgtool.exe":helper).wstring(),std::vector<std::wstring>{L"import",source.wstring(),task->output.wstring()},180000);
 sgImportTask=std::move(task);sgImportStatus="sg.import_running";sgImportLog("started");utility->AEGP_CauseIdleRoutinesToBeCalled();return 0;
}
PF_Err sgBeginImport(PF_InData* in){
 if(sgImportTask)throw std::runtime_error("sg.error.import_busy");
 wchar_t source[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();dialog.lpstrFile=source;dialog.nMaxFile=32768;dialog.lpstrFilter=L"RetroArch preset (*.slangp)\0*.slangp\0\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&dialog))return 0;
 return sgStartImport(in,source);
}
}
