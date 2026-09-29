#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace sentinel {

struct GlobalConfig {
    uint32_t polling_interval_ms{1000};
    std::string log_file_path{"logs/incidents.log"};
    bool enable_ansi_tui{true};
};

struct DetectionConfig {
    uint32_t consecutive_samples_required{3};
    uint64_t memory_leak_slope_threshold_kb{2048};
    uint64_t memory_max_rss_limit_kb{102400};
    double cpu_saturation_threshold_percent{85.0};
    size_t history_window_size{5};
};

struct RecoveryConfig {
    uint32_t graceful_stop_timeout_ms{3000};
    uint32_t circuit_breaker_max_retries{3};
    uint32_t circuit_breaker_window_ms{60000};
    uint32_t verification_stabilization_ms{2000};
    uint32_t diagnosis_cooldown_ms{5000};
};

struct ServiceConfig {
    std::string name;
    std::string executable;
    std::vector<std::string> arguments;
    std::vector<std::string> depends_on;
    bool allow_auto_recovery{true};
};

class ConfigManager {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    bool loadFromFile(const std::string& filepath);
    bool validate();

    const GlobalConfig& getGlobalConfig() const { return global_cfg_; }
    const DetectionConfig& getDetectionConfig() const { return detection_cfg_; }
    const RecoveryConfig& getRecoveryConfig() const { return recovery_cfg_; }
    const std::map<std::string, ServiceConfig>& getServices() const { return services_; }

    const std::string& getLastError() const { return last_error_; }

private:
    GlobalConfig global_cfg_;
    DetectionConfig detection_cfg_;
    RecoveryConfig recovery_cfg_;
    std::map<std::string, ServiceConfig> services_;
    std::string last_error_;

    static std::string trim(const std::string& s);
};

} // namespace sentinel
