#include "AnomalyDetector.h"
#include <numeric>

namespace sentinel {

AnomalyDetector::AnomalyDetector()
    : thresholds_(DetectionThresholds()) {}

AnomalyDetector::AnomalyDetector(const DetectionThresholds& thresholds)
    : thresholds_(thresholds) {}

void AnomalyDetector::resetHistory(const std::string& service_name) {
    history_windows_.erase(service_name);
    memory_leak_counter_.erase(service_name);
    cpu_spike_counter_.erase(service_name);
}

bool AnomalyDetector::detectMemoryLeak(const std::deque<ProcessMetrics>& window) {
    if (window.size() < 3) return false;

    // Check if memory has strictly increased across recent samples
    bool strictly_increasing = true;
    for (size_t i = 1; i < window.size(); ++i) {
        if (window[i].rss_kb <= window[i - 1].rss_kb) {
            strictly_increasing = false;
            break;
        }
    }

    if (strictly_increasing) {
        long net_growth = window.back().rss_kb - window.front().rss_kb;
        if (net_growth >= thresholds_.memory_leak_slope_kb) {
            return true;
        }
    }
    return false;
}

bool AnomalyDetector::detectCpuSaturation(const std::deque<ProcessMetrics>& window) {
    if (window.size() < 3) return false;

    double sum = 0.0;
    for (const auto& sample : window) {
        sum += sample.cpu_percent;
    }
    double avg = sum / window.size();
    return (avg >= thresholds_.cpu_saturation_percent);
}

std::optional<AnomalyReport> AnomalyDetector::evaluate(const ProcessMetrics& metrics) {
    AnomalyReport report;
    report.service_name = metrics.service_name;
    report.timestamp_ms = metrics.timestamp_ms;

    // 1. Immediate Critical Check: Process Termination
    if (!metrics.is_alive) {
        report.fault_type = FaultType::PROCESS_TERMINATED;
        report.severity = Severity::SEV_CRITICAL;
        report.consecutive_samples = 1;
        report.message = "Process has terminated unexpectedly or PID no longer exists.";
        resetHistory(metrics.service_name);
        return report;
    }

    // 2. Maintain Sliding Window
    auto& window = history_windows_[metrics.service_name];
    window.push_back(metrics);
    if (window.size() > thresholds_.window_size) {
        window.pop_front();
    }

    // 3. Absolute Memory Breach Check
    if (metrics.rss_kb >= thresholds_.max_rss_limit_kb) {
        report.fault_type = FaultType::HIGH_MEMORY;
        report.severity = Severity::SEV_CRITICAL;
        report.consecutive_samples = ++memory_leak_counter_[metrics.service_name];
        report.message = "Resident memory (" + std::to_string(metrics.rss_kb / 1024) + 
                         " MB) exceeds hard limit (" + 
                         std::to_string(thresholds_.max_rss_limit_kb / 1024) + " MB).";
        return report;
    }

    // 4. Memory Leak Trend Analysis
    if (detectMemoryLeak(window)) {
        int breaches = ++memory_leak_counter_[metrics.service_name];
        if (breaches >= thresholds_.consecutive_breaches_required) {
            report.fault_type = FaultType::MEMORY_GROWTH;
            report.severity = Severity::SEV_WARNING;
            report.consecutive_samples = breaches;
            long growth_kb = window.back().rss_kb - window.front().rss_kb;
            report.message = "Monotonic memory growth detected: +" + 
                             std::to_string(growth_kb / 1024) + " MB over " + 
                             std::to_string(window.size()) + " sample intervals.";
            return report;
        }
    } else {
        if (memory_leak_counter_[metrics.service_name] > 0) {
            memory_leak_counter_[metrics.service_name]--;
        }
    }

    // 5. CPU Saturation Analysis
    if (detectCpuSaturation(window)) {
        int breaches = ++cpu_spike_counter_[metrics.service_name];
        if (breaches >= thresholds_.consecutive_breaches_required) {
            report.fault_type = FaultType::CPU_SATURATION;
            report.severity = Severity::SEV_WARNING;
            report.consecutive_samples = breaches;
            report.message = "Sustained high CPU utilization exceeding " + 
                             std::to_string(static_cast<int>(thresholds_.cpu_saturation_percent)) + "%.";
            return report;
        }
    } else {
        if (cpu_spike_counter_[metrics.service_name] > 0) {
            cpu_spike_counter_[metrics.service_name]--;
        }
    }

    return std::nullopt; // No anomaly detected; system is healthy
}

} // namespace sentinel
