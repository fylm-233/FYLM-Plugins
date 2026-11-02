#pragma once
#include <windows.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace fylm::sg {
// No shell. Used by offline tools and UI-only asynchronous imports, never a render callback.
class ToolProcess {
 HANDLE job_=nullptr,process_=nullptr,read_=nullptr;ULONGLONG start_=0;DWORD timeout_=0;
 void drain(){char bytes[8192];DWORD available=0,read=0,total=0;while(total<65536&&PeekNamedPipe(read_,nullptr,0,nullptr,&available,nullptr)&&available){if(!ReadFile(read_,bytes,std::min<DWORD>(available,sizeof(bytes)),&read,nullptr)||!read)break;total+=read;auto count=std::min<std::size_t>(read,1024*1024-output.size());output.append(bytes,count);if(count<read)truncated=true;}}
public:
 bool running=false,timedOut=false,truncated=false;DWORD exitCode=STILL_ACTIVE;std::string output;
 static std::wstring quote(const std::wstring& argument){std::wstring result=L"\"";unsigned slash=0;for(auto c:argument){if(c==L'\\'){slash++;continue;}if(c==L'"'){result.append(slash*2+1,L'\\');result+=c;}else{result.append(slash,L'\\');result+=c;}slash=0;}result.append(slash*2,L'\\');return result+L"\"";}
 ToolProcess(const std::wstring& exe,const std::vector<std::wstring>& args,DWORD timeout=120000){
  if(timeout<50||timeout>600000)throw std::runtime_error("sg.error.timeout");timeout_=timeout;
  SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};HANDLE write=nullptr;
  try{if(!CreatePipe(&read_,&write,&security,0)||!SetHandleInformation(read_,HANDLE_FLAG_INHERIT,0))throw std::runtime_error("sg.error.worker");
   job_=CreateJobObjectW(nullptr,nullptr);if(!job_)throw std::runtime_error("sg.error.worker");JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_PROCESS_MEMORY;MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);limits.ProcessMemoryLimit=2ull*1024*1024*1024;if(GlobalMemoryStatusEx(&memory))limits.ProcessMemoryLimit=std::min<SIZE_T>(limits.ProcessMemoryLimit,std::max<SIZE_T>(64*1024*1024,memory.ullAvailPhys/4));if(!SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("sg.error.worker");
   std::wstring command=quote(exe);for(auto& arg:args)command+=L" "+quote(arg);std::vector<wchar_t> writable(command.begin(),command.end());writable.push_back(0);STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;startup.hStdOutput=startup.hStdError=write;PROCESS_INFORMATION child{};
   if(!CreateProcessW(exe.c_str(),writable.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,nullptr,&startup,&child))throw std::runtime_error("sg.error.worker");process_=child.hProcess;
   if(!AssignProcessToJobObject(job_,process_)){TerminateProcess(process_,1);CloseHandle(child.hThread);throw std::runtime_error("sg.error.worker");}if(ResumeThread(child.hThread)==DWORD(-1)){TerminateProcess(process_,1);CloseHandle(child.hThread);throw std::runtime_error("sg.error.worker");}CloseHandle(child.hThread);CloseHandle(write);write=nullptr;running=true;start_=GetTickCount64();
  }catch(...){if(write)CloseHandle(write);if(process_){TerminateProcess(process_,1);CloseHandle(process_);process_=nullptr;}if(read_){CloseHandle(read_);read_=nullptr;}if(job_){CloseHandle(job_);job_=nullptr;}throw;}
 }
 bool poll(){if(!running)return true;drain();if(WaitForSingleObject(process_,0)==WAIT_OBJECT_0){GetExitCodeProcess(process_,&exitCode);running=false;drain();return true;}if(GetTickCount64()-start_>timeout_){TerminateJobObject(job_,124);WaitForSingleObject(process_,100);timedOut=true;exitCode=124;running=false;drain();return true;}return false;}
 ~ToolProcess(){if(running&&job_)TerminateJobObject(job_,125);if(process_)CloseHandle(process_);if(read_)CloseHandle(read_);if(job_)CloseHandle(job_);}
 ToolProcess(const ToolProcess&)=delete;ToolProcess& operator=(const ToolProcess&)=delete;
};
}
