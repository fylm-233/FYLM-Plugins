#include "pixel_art/core.hpp"
#include "pixel_art/execution.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
namespace pixel_art {
Result renderSerial(ConstImageView,ImageView,const Params&,IProgress*)noexcept;
namespace {
// Share helpers across simultaneous frames; never wait for a permit while AE
// holds a frame. Under contention a frame simply keeps the serial path.
std::atomic<int> helpers{0};
struct Permits {
 int count=0;
 Permits(int wanted,int limit){int observed=helpers.load();while(observed<limit){int take=std::min(wanted,limit-observed);if(helpers.compare_exchange_weak(observed,observed+take)){count=take;break;}}}
 ~Permits(){helpers.fetch_sub(count);}
};
struct Shared {
 std::atomic<bool> stop{false};std::atomic<int> completed{0};std::mutex mutex;std::condition_variable changed;
};
struct WorkerProgress:IProgress {
 Shared& shared;explicit WorkerProgress(Shared& s):shared(s){}
 bool cancelled()override{return shared.stop.load(std::memory_order_relaxed);}
 void report(double)override{}
};
struct CallerProgress:IProgress {
 Shared& shared;IProgress* caller;int count;
 CallerProgress(Shared& s,IProgress* p,int n):shared(s),caller(p),count(n){}
 bool cancelled()override{if(caller&&caller->cancelled())shared.stop=true;return shared.stop;}
 void report(double f)override{if(caller)caller->report(f/count);}
};
struct Join {
 std::vector<std::thread>& threads;~Join(){for(auto& t:threads)if(t.joinable())t.join();}
};
}
Result render(ConstImageView src,ImageView dst,const Params& p,IProgress* progress)noexcept{
 if(p.workers<0||p.workers>8)return {false,ErrorCode::InvalidParams,errorKey(ErrorCode::InvalidParams)};
 // Restrict partitioning to exact binary preview scales and integral origins;
 // other coordinate cases continue through the proven serial implementation.
 bool scale=p.grid.downsampleY==1||p.grid.downsampleY==.5||p.grid.downsampleY==.25||p.grid.downsampleY==.125;
 if(p.workers==1||!p.pc98||p.style!=Style::Basic||p.outlineStrength!=0||!scale||dst.width<1||dst.height<128||dst.width>32768||dst.height>32768||std::uint64_t(dst.width)*dst.height<262144||std::floor(dst.originY)!=dst.originY||src.height<1||src.height>32768||!src.data||!dst.data||src.rowBytes>SIZE_MAX/std::size_t(src.height)||dst.rowBytes>SIZE_MAX/std::size_t(dst.height))return renderSerial(src,dst,p,progress);
 auto a=reinterpret_cast<std::uintptr_t>(src.data),b=reinterpret_cast<std::uintptr_t>(dst.data);
 if(a<=b?b-a<src.rowBytes*src.height:a-b<dst.rowBytes*dst.height)return renderSerial(src,dst,p,progress);
 if(!std::isfinite(p.grid.downsampleX)||p.grid.downsampleX<=0||p.grid.block<1)return renderSerial(src,dst,p,progress);
 // Conservative per-band upper bound, including fixed caches and all row planes.
 bool direct=p.pc98ColorSpec==PC98ColorSpec::RGB565||p.pc98ColorSpec==PC98ColorSpec::RGB888;
 long double perBand=(std::ceil(src.width/p.grid.downsampleX/p.grid.block)+4)*(direct?175:171)+dst.width*16.L+512*1024;
 auto capacity=cpuCapacity();int hardwareLimit=executionWorkers(capacity);int wanted=std::min(executionWorkers(capacity,p.workers)-1,dst.height/64-1);
 auto parallelBudget=p.memoryBudget;if(capacity.availableMemory)parallelBudget=std::min<std::uint64_t>(parallelBudget,capacity.availableMemory/16);
 while(wanted>0&&perBand*(wanted+1)>parallelBudget)wanted--;
 if(wanted<=0)return renderSerial(src,dst,p,progress);
 Permits permits(wanted,hardwareLimit-1);if(!permits.count)return renderSerial(src,dst,p,progress);
 try{
  const int count=permits.count+1;Shared shared;std::vector<Result> results(count);std::vector<std::thread> threads;threads.reserve(count-1);
  Params serial=p;serial.workers=1;serial.memoryBudget=p.memoryBudget/std::size_t(count);
  auto band=[&](int i){auto view=dst;int first=dst.height*i/count,last=dst.height*(i+1)/count;view.data=static_cast<unsigned char*>(dst.data)+std::size_t(first)*dst.rowBytes;view.height=last-first;view.originY=dst.originY+first/p.grid.downsampleY;return view;};
  Join join{threads};
  try{for(int i=1;i<count;i++)threads.emplace_back([&,i]{WorkerProgress monitor(shared);results[i]=renderSerial(src,band(i),serial,&monitor);if(!results[i].ok)shared.stop=true;shared.completed.fetch_add(1);shared.changed.notify_one();});}
  catch(...){shared.stop=true;for(auto& t:threads)t.join();return renderSerial(src,dst,p,progress);}
  CallerProgress monitor(shared,progress,count);results[0]=renderSerial(src,band(0),serial,&monitor);if(!results[0].ok)shared.stop=true;
  // Only the original caller invokes host progress/abort callbacks.
  while(shared.completed.load()<count-1){monitor.cancelled();std::unique_lock<std::mutex> lock(shared.mutex);shared.changed.wait_for(lock,std::chrono::milliseconds(5));}
  for(auto& t:threads)t.join();
  for(const auto& result:results)if(!result.ok&&result.code!=ErrorCode::Cancelled)return result;
  for(const auto& result:results)if(!result.ok)return result;
  if(monitor.cancelled())return {false,ErrorCode::Cancelled,errorKey(ErrorCode::Cancelled)};
  if(progress)progress->report(1);return {true,ErrorCode::None,{}};
 }catch(const std::bad_alloc&){return {false,ErrorCode::OutOfMemory,errorKey(ErrorCode::OutOfMemory)};}catch(...){return {false,ErrorCode::Internal,errorKey(ErrorCode::Internal)};}
}
}
