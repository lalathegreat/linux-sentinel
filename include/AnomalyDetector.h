#ifndef LINUX_SENTINEL_ANOMALY_DETECTOR_H
#define LINUX_SENTINEL_ANOMALY_DETECTOR_H

#include "Common.h"
#include <deque>
#include <unordered_map>
#include <optional>
#include <string>

namespace sentinel {

struct DetectionThresholds {
    size_t window_size{5};
    int consecutive_breaches_required{3};
    long max_rss_limit_kb{102400};        // 100 MB default
    long memory_leak_slope_kb{2048};       // > 2MB continuous growth
    double cpu_saturation_percent{85.0};  // > 85% CPU average
};

class AnomalyDetector {
public:
    AnomalyDetector();
    explicit AnomalyDetector(const DetectionThresholds& thresholds);
    ~AnomalyDetector() = default;

    // Ingest a telemetry sample and evaluate for anomalies
    std::optional<AnomalyReport> evaluate(const ProcessMetrics& metrics);

    // Reset history for a service (e.g. after successful restart)
    void resetHistory(const std::string& service_name);

private:
    DetectionThresholds thresholds_;
    
    // Per-service sliding window of past metrics
    std::unordered_map<std::string, std::deque<ProcessMetrics>> history_windows_;

    // Per-service consecutive violation counters
    std::unordered_map<std::string, int> memory_leak_counter_;
    std::unordered_map<std::string, int> cpu_spike_counter_;

    bool detectMemoryLeak(const std::deque<ProcessMetrics>& window);
    bool detectCpuSaturation(const std::deque<ProcessMetrics>& window);
};

} // namespace sentinel

#endif // LINUX_SENTINEL_ANOMALY_DETECTOR_H
