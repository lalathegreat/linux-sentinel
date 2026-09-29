#include "HealthCollector.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cerrno>

#if defined(__APPLE__)
#include <libproc.h>
#include <mach/mach.h>
#endif

namespace sentinel {

HealthCollector::HealthCollector() {
    long ticks = sysconf(_SC_CLK_TCK);
    clock_ticks_per_sec_ = (ticks > 0) ? ticks : 100;
}

bool HealthCollector::isProcessAlive(int pid) {
    if (pid <= 0) return false;

    // First check if this is our child that has terminated/zombied
    int status = 0;
    pid_t w = waitpid(pid, &status, WNOHANG);
    if (w == pid || (w == -1 && errno == ECHILD)) {
        return false; // Process has exited
    }

    // Next check via POSIX kill signal 0
    if (kill(pid, 0) != 0) {
        return false;
    }

    return true;
}

void HealthCollector::clearProcessCache(int pid) {
    previous_samples_.erase(pid);
}

bool HealthCollector::parseProcStat(int pid, ProcessMetrics& metrics) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    if (!std::getline(file, line)) return false;

    auto open_paren = line.find('(');
    auto close_paren = line.rfind(')');
    if (open_paren == std::string::npos || close_paren == std::string::npos || close_paren < open_paren) {
        return false;
    }

    std::string rest = line.substr(close_paren + 2);
    std::istringstream iss(rest);

    char state_char = 'R';
    int ppid = 0, pgrp = 0, session = 0, tty_nr = 0, tpgid = 0;
    unsigned int flags = 0;
    unsigned long minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
    unsigned long utime = 0, stime = 0;
    long cutime = 0, cstime = 0, priority = 0, nice = 0, num_threads = 1;

    if (iss >> state_char >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags
            >> minflt >> cminflt >> majflt >> cmajflt >> utime >> stime
            >> cutime >> cstime >> priority >> nice >> num_threads) {
        metrics.state_char = state_char;
        metrics.utime_ticks = utime;
        metrics.stime_ticks = stime;
        metrics.num_threads = num_threads;
        return true;
    }

    return false;
}

bool HealthCollector::parseProcStatus(int pid, ProcessMetrics& metrics) {
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream iss(line.substr(6));
            long rss_kb = 0;
            if (iss >> rss_kb) metrics.rss_kb = rss_kb;
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream iss(line.substr(7));
            long vmsize_kb = 0;
            if (iss >> vmsize_kb) metrics.vmsize_kb = vmsize_kb;
        } else if (line.rfind("Threads:", 0) == 0) {
            std::istringstream iss(line.substr(8));
            long threads = 0;
            if (iss >> threads && metrics.num_threads <= 1) metrics.num_threads = threads;
        }
    }
    return true;
}

bool HealthCollector::parsePosixFallback(int pid, ProcessMetrics& metrics) {
#if defined(__APPLE__)
    struct proc_taskinfo tinfo{};
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo, sizeof(tinfo));
    if (ret <= 0) return false;

    metrics.rss_kb = tinfo.pti_resident_size / 1024;
    metrics.vmsize_kb = tinfo.pti_virtual_size / 1024;
    metrics.num_threads = tinfo.pti_threadnum;
    metrics.state_char = 'R';

    uint64_t total_ticks = (tinfo.pti_total_user + tinfo.pti_total_system) / (1000000000ULL / clock_ticks_per_sec_);
    metrics.utime_ticks = total_ticks;
    metrics.stime_ticks = 0;
    return true;
#else
    (void)pid;
    (void)metrics;
    return false;
#endif
}

ProcessMetrics HealthCollector::pollProcess(int pid, const std::string& service_name) {
    ProcessMetrics metrics;
    metrics.pid = pid;
    metrics.service_name = service_name;
    metrics.timestamp_ms = getCurrentTimeMs();
    metrics.is_alive = isProcessAlive(pid);

    if (!metrics.is_alive) {
        metrics.state_char = 'Z';
        previous_samples_.erase(pid);
        return metrics;
    }

    // 1. Attempt Linux /proc ingestion
    bool success = parseProcStat(pid, metrics) && parseProcStatus(pid, metrics);

    // 2. If /proc is not available (macOS development host), use fallback
    if (!success) {
        success = parsePosixFallback(pid, metrics);
    }

    // If telemetry reading failed, process has likely exited or zombied
    if (!success) {
        metrics.is_alive = false;
        metrics.state_char = 'Z';
        previous_samples_.erase(pid);
        return metrics;
    }

    // 3. Compute differential CPU percentage
    auto it = previous_samples_.find(pid);
    if (it != previous_samples_.end()) {
        const CpuSample& prev = it->second;
        uint64_t delta_ticks = (metrics.utime_ticks + metrics.stime_ticks) - 
                              (prev.utime_ticks + prev.stime_ticks);
        double delta_sec = (metrics.timestamp_ms - prev.timestamp_ms) / 1000.0;

        if (delta_sec > 0.05 && clock_ticks_per_sec_ > 0) {
            double percent = ((static_cast<double>(delta_ticks) / clock_ticks_per_sec_) / delta_sec) * 100.0;
            metrics.cpu_percent = (percent < 0.0) ? 0.0 : percent;
        }
    }

    // Cache current sample
    previous_samples_[pid] = {metrics.utime_ticks, metrics.stime_ticks, metrics.timestamp_ms};

    return metrics;
}

} // namespace sentinel
