#pragma once
#include <cstdint>
namespace pixel_art {
struct CpuCapacity {
 unsigned logical=1,physical=0;
 std::uint64_t availableMemory=0; // 0 means unavailable, never an invented size.
};
CpuCapacity cpuCapacity()noexcept;
// Conservative policy: reserve one available physical core, hard ceiling eight.
// Unknown topology falls back to one; requested=0 selects the detected policy.
int executionWorkers(CpuCapacity,int requested=0)noexcept;
}
