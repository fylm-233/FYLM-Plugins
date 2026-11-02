#include "pixel_art/execution.hpp"
#include <algorithm>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#endif
namespace pixel_art {
int executionWorkers(CpuCapacity capacity,int requested)noexcept{
 unsigned physical=std::min(capacity.logical,capacity.physical);
 unsigned available=physical>2?physical-1:1;
 int limit=int(std::min(8u,available));
 return requested>0?std::max(1,std::min(requested,limit)):limit;
}
CpuCapacity cpuCapacity()noexcept{
 CpuCapacity result;
#ifdef _WIN32
 try{
  MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(GlobalMemoryStatusEx(&memory))result.availableMemory=memory.ullAvailPhys;
  GROUP_AFFINITY affinity{};if(!GetThreadGroupAffinity(GetCurrentThread(),&affinity))return result;
  DWORD_PTR process=0,system=0;
  if(GetProcessAffinityMask(GetCurrentProcess(),&process,&system)&&process)affinity.Mask&=process;
  auto bits=[](KAFFINITY mask){unsigned count=0;while(mask){count++;mask&=mask-1;}return count;};
  result.logical=std::max(1u,bits(affinity.Mask));
  DWORD size=0;GetLogicalProcessorInformationEx(RelationProcessorCore,nullptr,&size);if(!size)return result;
  std::vector<unsigned char> topology(size);
  if(!GetLogicalProcessorInformationEx(RelationProcessorCore,reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(topology.data()),&size))return result;
  for(std::size_t offset=0;offset<size;){
   auto entry=reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(topology.data()+offset);
   if(!entry->Size||entry->Size>size-offset)break;
   if(entry->Relationship==RelationProcessorCore){bool eligible=false;for(WORD group=0;group<entry->Processor.GroupCount;group++){auto mask=entry->Processor.GroupMask[group];if(mask.Group==affinity.Group&&(mask.Mask&affinity.Mask))eligible=true;}if(eligible)result.physical++;}
   offset+=entry->Size;
  }
 }catch(...){result.physical=0;}
#endif
 return result;
}
}
