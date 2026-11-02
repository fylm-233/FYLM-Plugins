#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdint>
namespace retro::ae {
// Opt-in local diagnostic only. No global mutable state, no I/O without this environment variable.
// Used to prove actual AE callback overlap, rather than infer MFR from an enabled checkbox.
class RenderAudit {
 wchar_t path[1024]{};std::int64_t value=0;std::uint64_t scale=1;LONGLONG token=0;DWORD thread=0;
 void write(const char* kind) noexcept {
  if(!path[0])return;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
  HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(f==INVALID_HANDLE_VALUE)return;OVERLAPPED lock{};
  if(LockFileEx(f,LOCKFILE_EXCLUSIVE_LOCK,0,1,0,&lock)){
   char line[240];int n=std::snprintf(line,sizeof(line),"%s,%lld,%lu,%lld,%lld,%llu\n",kind,now.QuadPart,static_cast<unsigned long>(thread),token,static_cast<long long>(value),static_cast<unsigned long long>(scale));
   LARGE_INTEGER end{};SetFilePointerEx(f,end,nullptr,FILE_END);DWORD bytes=0;if(n>0&&n<int(sizeof(line)))WriteFile(f,line,DWORD(n),&bytes,nullptr);
   UnlockFileEx(f,0,1,0,&lock);
  }CloseHandle(f);
 }
public:
 RenderAudit(std::int64_t time,std::uint64_t timeScale) noexcept:value(time),scale(timeScale){
  DWORD n=GetEnvironmentVariableW(L"DITHER_MFR_AUDIT",path,DWORD(sizeof(path)/sizeof(path[0])));if(!n||n>=sizeof(path)/sizeof(path[0])){path[0]=0;return;}
  LARGE_INTEGER now{};QueryPerformanceCounter(&now);token=now.QuadPart;thread=GetCurrentThreadId();write("begin");
 }
 ~RenderAudit(){write("end");}
};
}

