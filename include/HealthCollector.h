#ifndef LINUX_SENTINEL_HEALTH_COLLECTOR_H
#define LINUX_SENTINEL_HEALTH_COLLECTOR_H

#include "Common.h"
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sentinel {

class HealthCollector {
public:
    HealthCollector();
    ~HealthCollector() = default;

    // Check whether a process is alive using POSIX signaling
    bool isProcessAlive(int pid);

    // Collect telemetry for target PID
    ProcessMetrics pollProcess(int pid, const std::string& service_name);

    // Reset cached history for a PID
    void clearProcessCache(int pid);

private:
    struct CpuSample {
        uint64_t utime_ticks{0};
        uint64_t stime_ticks{0};
        uint64_t timestamp_ms{0};
    };

    // Cached previous sample per PID for differential CPU% calculation
    std::unordered_map<int, CpuSample> previous_samples_;
    long clock_ticks_per_sec_{100};

    // Internal Linux /proc parsers
    bool parseProcStat(int pid, ProcessMetrics& metrics);
    bool parseProcStatus(int pid, ProcessMetrics& metrics);

    // Portable POSIX / macOS fallback when /proc is unavailable
    bool parsePosixFallback(int pid, ProcessMetrics& metrics);
};

} // namespace sentinel

#endif // LINUX_SENTINEL_HEALTH_COLLECTOR_H
