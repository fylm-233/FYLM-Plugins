#include "dither/core.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif
int main(int argc, char **argv) {
    std::ofstream csv(argc > 1 ? argv[1] : "benchmark.csv");
    csv << "algorithm,width,height,working_resolution,bit_depth,backend,frame_ms,peak_mb,host_cpu,ae_"
           "version\n";
    for (int size : {512, 1920, 3840})
        for (int resolution : {512, 0})
            for (int id : {1, 2, 8, 15, 17}) {
                int h = size == 512 ? 512 : size * 9 / 16;
                std::vector<unsigned char> s(size_t(size) * h * 4), d(s.size());
                for (size_t i = 0; i < s.size(); ++i)
                    s[i] = static_cast<unsigned char>(i * 17 + i / 31);
                dither::RenderParams p;
                p.algorithmId = id;
                p.workingResolution = resolution;
                auto start = std::chrono::steady_clock::now();
                auto r = dither::render({s.data(), size, h, size_t(size) * 4, dither::PixelFormat::RGBA8},
                                        {d.data(), size, h, size_t(size) * 4, dither::PixelFormat::RGBA8}, p);
                if (!r.ok)
                    return 1;
                double ms =
                           std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                               .count(),
                       peak = 0;
#ifdef _WIN32
                PROCESS_MEMORY_COUNTERS pm{};
                GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm));
                peak = pm.PeakWorkingSetSize / 1048576.0;
#endif
                csv << id << ',' << size << ',' << h << ',' << resolution << ",8,CPU," << ms << ',' << peak
                    << ",see-report,CLI\n";
            }
    std::cout << "30 measured frames written. peak_mb is process cumulative high-water mark.\n";
}
