#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace sentinel {

std::string ConfigManager::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    last_error_.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        last_error_ = "Failed to open configuration file: '" + filepath + "' (No such file or directory).";
        return false;
    }

    std::string line;
    std::string current_section;
    uint32_t line_number = 0;

    std::map<std::string, std::map<std::string, std::string>> raw_services;

    while (std::getline(file, line)) {
        line_number++;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue; // Skip comments and blank lines
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current_section = trim(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }

        auto eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            last_error_ = "Syntax error on line " + std::to_string(line_number) + 
                          ": missing '=' in key-value assignment: '" + trimmed + "'";
            return false;
        }

        std::string key = trim(trimmed.substr(0, eq_pos));
        std::string val = trim(trimmed.substr(eq_pos + 1));

        if (key.empty()) {
            last_error_ = "Syntax error on line " + std::to_string(line_number) + ": empty key name.";
            return false;
        }

        try {
            if (current_section == "global") {
                if (key == "polling_interval_ms") {
                    global_cfg_.polling_interval_ms = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "log_file_path") {
                    global_cfg_.log_file_path = val;
                } else if (key == "enable_ansi_tui") {
                    global_cfg_.enable_ansi_tui = (val == "true" || val == "1" || val == "yes");
                }
            } else if (current_section == "detection_thresholds") {
                if (key == "consecutive_samples_required") {
                    detection_cfg_.consecutive_samples_required = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "memory_leak_slope_threshold_kb") {
                    detection_cfg_.memory_leak_slope_threshold_kb = std::stoull(val);
                } else if (key == "memory_max_rss_limit_kb") {
                    detection_cfg_.memory_max_rss_limit_kb = std::stoull(val);
                } else if (key == "cpu_saturation_threshold_percent") {
                    detection_cfg_.cpu_saturation_threshold_percent = std::stod(val);
                } else if (key == "history_window_size") {
                    detection_cfg_.history_window_size = std::stoul(val);
                }
            } else if (current_section == "recovery_policy") {
                if (key == "graceful_stop_timeout_ms") {
                    recovery_cfg_.graceful_stop_timeout_ms = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "circuit_breaker_max_retries") {
                    recovery_cfg_.circuit_breaker_max_retries = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "circuit_breaker_window_ms") {
                    recovery_cfg_.circuit_breaker_window_ms = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "verification_stabilization_ms") {
                    recovery_cfg_.verification_stabilization_ms = static_cast<uint32_t>(std::stoul(val));
                } else if (key == "diagnosis_cooldown_ms") {
                    recovery_cfg_.diagnosis_cooldown_ms = static_cast<uint32_t>(std::stoul(val));
                }
            } else if (current_section == "services") {
                // Format: service.<id>.<property>
                if (key.rfind("service.", 0) == 0) {
                    auto second_dot = key.find('.', 8);
                    if (second_dot != std::string::npos) {
                        std::string svc_id = key.substr(8, second_dot - 8);
                        std::string prop = key.substr(second_dot + 1);
                        raw_services[svc_id][prop] = val;
                    }
                }
            }
        } catch (const std::exception& e) {
            last_error_ = "Malformed value on line " + std::to_string(line_number) + 
                          " for key '" + key + "': " + e.what();
            return false;
        }
    }

    // Assemble raw services
    services_.clear();
    for (const auto& [svc_id, props] : raw_services) {
        ServiceConfig svc;
        if (props.find("name") != props.end()) {
            svc.name = props.at("name");
        }
        if (props.find("executable") != props.end()) {
            svc.executable = props.at("executable");
        }
        if (props.find("arguments") != props.end()) {
            std::string args_str = props.at("arguments");
            std::istringstream iss(args_str);
            std::string arg;
            while (iss >> arg) {
                svc.arguments.push_back(arg);
            }
        }
        if (props.find("depends_on") != props.end()) {
            std::string deps_str = props.at("depends_on");
            if (deps_str != "none" && !deps_str.empty()) {
                std::istringstream iss(deps_str);
                std::string dep;
                while (std::getline(iss, dep, ',')) {
                    std::string trimmed_dep = trim(dep);
                    if (!trimmed_dep.empty()) {
                        svc.depends_on.push_back(trimmed_dep);
                    }
                }
            }
        }
        if (props.find("allow_auto_recovery") != props.end()) {
            std::string v = props.at("allow_auto_recovery");
            svc.allow_auto_recovery = (v == "true" || v == "1" || v == "yes");
        }

        if (!svc.name.empty()) {
            services_[svc.name] = svc;
        }
    }

    return validate();
}

bool ConfigManager::validate() {
    if (global_cfg_.polling_interval_ms < 50 || global_cfg_.polling_interval_ms > 60000) {
        last_error_ = "Validation failed: polling_interval_ms (" + 
                      std::to_string(global_cfg_.polling_interval_ms) + 
                      ") must be between 50ms and 60000ms.";
        return false;
    }
    if (global_cfg_.log_file_path.empty()) {
        last_error_ = "Validation failed: log_file_path must not be empty.";
        return false;
    }
    if (detection_cfg_.consecutive_samples_required < 1) {
        last_error_ = "Validation failed: consecutive_samples_required must be >= 1.";
        return false;
    }
    if (detection_cfg_.cpu_saturation_threshold_percent <= 0.0 || 
        detection_cfg_.cpu_saturation_threshold_percent > 1000.0) {
        last_error_ = "Validation failed: cpu_saturation_threshold_percent must be in range (0.0, 1000.0].";
        return false;
    }
    if (recovery_cfg_.circuit_breaker_max_retries < 1) {
        last_error_ = "Validation failed: circuit_breaker_max_retries must be >= 1.";
        return false;
    }
    if (recovery_cfg_.verification_stabilization_ms == 0) {
        last_error_ = "Validation failed: verification_stabilization_ms must be > 0.";
        return false;
    }

    for (const auto& [name, svc] : services_) {
        if (svc.name.empty()) {
            last_error_ = "Validation failed: Service has an empty name.";
            return false;
        }
        if (svc.executable.empty()) {
            last_error_ = "Validation failed: Service '" + name + "' has an empty executable path.";
            return false;
        }
    }

    return true;
}

} // namespace sentinel
