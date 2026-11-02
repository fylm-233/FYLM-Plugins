#pragma once
#include <windows.h>
#include <functional>
#include <stdexcept>
#include <cstdio>
namespace retro {
// Same named kernel mutex in each FYLM .aex: CPU core count is not AE concurrency.
// Bound active staging work across products. Backends drop frame-sized resources
// before releasing this lease; small per-worker devices/program caches remain.
class WorkspaceLease {
 HANDLE handle=nullptr;bool owned=false;
public:
 static constexpr std::size_t limit=1536ull*1024*1024;
 WorkspaceLease(){wchar_t name[96];std::swprintf(name,96,L"Local\\FYLM-Staging-%lu",GetCurrentProcessId());handle=CreateMutexW(nullptr,FALSE,name);if(!handle)throw std::runtime_error("workspace mutex");}
 bool acquire(const std::function<bool()>& cancelled){
  for(;;){if(cancelled&&cancelled())return false;DWORD result=WaitForSingleObject(handle,25);
   if(result==WAIT_OBJECT_0||result==WAIT_ABANDONED){owned=true;return true;}
   if(result!=WAIT_TIMEOUT)throw std::runtime_error("workspace wait");}
 }
 ~WorkspaceLease(){if(owned)ReleaseMutex(handle);if(handle)CloseHandle(handle);}
 WorkspaceLease(const WorkspaceLease&)=delete;WorkspaceLease& operator=(const WorkspaceLease&)=delete;
};
}
