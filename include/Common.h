#ifndef LINUX_SENTINEL_COMMON_H
#define LINUX_SENTINEL_COMMON_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <map>
#include <deque>
#include <optional>

namespace sentinel {

// Service Operational Lifecycle States
enum class ServiceState {
    STOPPED,
    HEALTHY,
    WARNING,
    CRITICAL,
    CHECKPOINTING,
    RECOVERING,
    VERIFYING,
    SAFE_MODE
};

// Classified Fault Categories
enum class FaultType {
    NONE,
    PROCESS_TERMINATED,
    HIGH_MEMORY,
    MEMORY_GROWTH,
    CPU_SATURATION,
    DEPENDENCY_FAILURE,
    RECOVERY_TIMEOUT
};

// Severity Levels
enum class Severity {
    SEV_INFO,
    SEV_WARNING,
    SEV_CRITICAL,
    SEV_FATAL
};

// Orchestrated Recovery Action Types
enum class RecoveryActionType {
    ACTION_NONE,
    ACTION_GRACEFUL_RESTART,
    ACTION_FORCEFUL_RESTART,
    ACTION_ENTER_SAFE_MODE
};

// Outcome of a Recovery Execution
enum class RecoveryStatus {
    STATUS_PENDING,
    STATUS_SUCCESS,
    STATUS_FAILED,
    STATUS_CIRCUIT_BREAKER_TRIPPED
};

// Utility function to get current timestamp in milliseconds
inline uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Format millisecond epoch timestamp into human-readable ISO string
inline std::string formatTimestamp(uint64_t timestamp_ms) {
    std::time_t seconds = static_cast<std::time_t>(timestamp_ms / 1000);
    uint32_t ms = static_cast<uint32_t>(timestamp_ms % 1000);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &seconds);
#else
    gmtime_r(&seconds, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms << " UTC";
    return oss.str();
}

// Conversion Helpers
inline std::string serviceStateToString(ServiceState state) {
    switch (state) {
        case ServiceState::STOPPED: return "STOPPED";
        case ServiceState::HEALTHY: return "HEALTHY";
        case ServiceState::WARNING: return "WARNING";
        case ServiceState::CRITICAL: return "CRITICAL";
        case ServiceState::CHECKPOINTING: return "CHECKPOINTING";
        case ServiceState::RECOVERING: return "RECOVERING";
        case ServiceState::VERIFYING: return "VERIFYING";
        case ServiceState::SAFE_MODE: return "SAFE_MODE";
        default: return "UNKNOWN";
    }
}

inline std::string faultTypeToString(FaultType type) {
    switch (type) {
        case FaultType::NONE: return "NONE";
        case FaultType::PROCESS_TERMINATED: return "PROCESS_TERMINATED";
        case FaultType::HIGH_MEMORY: return "HIGH_MEMORY";
        case FaultType::MEMORY_GROWTH: return "MEMORY_GROWTH";
        case FaultType::CPU_SATURATION: return "CPU_SATURATION";
        case FaultType::DEPENDENCY_FAILURE: return "DEPENDENCY_FAILURE";
        case FaultType::RECOVERY_TIMEOUT: return "RECOVERY_TIMEOUT";
        default: return "UNKNOWN";
    }
}

inline std::string severityToString(Severity sev) {
    switch (sev) {
        case Severity::SEV_INFO: return "INFO";
        case Severity::SEV_WARNING: return "WARNING";
        case Severity::SEV_CRITICAL: return "CRITICAL";
        case Severity::SEV_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

inline std::string recoveryActionToString(RecoveryActionType action) {
    switch (action) {
        case RecoveryActionType::ACTION_NONE: return "NONE";
        case RecoveryActionType::ACTION_GRACEFUL_RESTART: return "GRACEFUL_RESTART";
        case RecoveryActionType::ACTION_FORCEFUL_RESTART: return "FORCEFUL_RESTART";
        case RecoveryActionType::ACTION_ENTER_SAFE_MODE: return "ENTER_SAFE_MODE";
        default: return "UNKNOWN";
    }
}

inline std::string recoveryStatusToString(RecoveryStatus status) {
    switch (status) {
        case RecoveryStatus::STATUS_PENDING: return "PENDING";
        case RecoveryStatus::STATUS_SUCCESS: return "SUCCESS";
        case RecoveryStatus::STATUS_FAILED: return "FAILED";
        case RecoveryStatus::STATUS_CIRCUIT_BREAKER_TRIPPED: return "CIRCUIT_BREAKER_TRIPPED";
        default: return "UNKNOWN";
    }
}

// Process Telemetry Snapshot collected from /proc
struct ProcessMetrics {
    int pid{0};
    std::string service_name;
    bool is_alive{false};
    char state_char{'?'};
    double cpu_percent{0.0};
    long rss_kb{0};
    long vmsize_kb{0};
    long num_threads{0};
    uint64_t utime_ticks{0};
    uint64_t stime_ticks{0};
    uint64_t timestamp_ms{0};

    std::string to_string() const {
        std::ostringstream oss;
        oss << "[PID: " << pid << " | " << service_name 
            << " | State: " << state_char
            << " | CPU: " << std::fixed << std::setprecision(1) << cpu_percent << "%"
            << " | RSS: " << (rss_kb / 1024) << " MB"
            << " | Threads: " << num_threads << "]";
        return oss.str();
    }
};

// Raw Anomaly Detection Output
struct AnomalyReport {
    std::string service_name;
    FaultType fault_type{FaultType::NONE};
    Severity severity{Severity::SEV_INFO};
    int consecutive_samples{0};
    std::string message;
    uint64_t timestamp_ms{0};
};

// Diagnosis Engine Classified Output
struct DiagnosisRecord {
    std::string service_name;
    FaultType diagnosed_fault{FaultType::NONE};
    Severity severity{Severity::SEV_INFO};
    RecoveryActionType recommended_action{RecoveryActionType::ACTION_NONE};
    std::string root_cause_evidence;
    std::string blocked_by_dependency;
    uint64_t timestamp_ms{0};
};

// Pre-Recovery Forensic Incident Snapshot
struct IncidentSnapshot {
    std::string incident_id;
    uint64_t timestamp_ms{0};
    std::string service_name;
    int target_pid{0};
    ProcessMetrics metrics;
    std::string config_version{"1.0.0"};
    std::string trigger_reason;
};

// Post-Recovery Verification Result
struct VerificationResult {
    bool is_successful{false};
    int new_pid{0};
    std::string verification_notes;
    double stabilization_time_sec{0.0};
    uint64_t timestamp_ms{0};
};

// Complete Auditable Incident Record
struct IncidentCard {
    std::string incident_id;
    std::string service_name;
    uint64_t start_time_ms{0};
    uint64_t end_time_ms{0};
    FaultType initial_fault{FaultType::NONE};
    IncidentSnapshot snapshot;
    RecoveryActionType action_taken{RecoveryActionType::ACTION_NONE};
    int retry_count{0};
    RecoveryStatus status{RecoveryStatus::STATUS_PENDING};
    VerificationResult verification;

    std::string to_formatted_card() const {
        std::ostringstream oss;
        oss << "========================================================\n"
            << "                INCIDENT CARD: " << incident_id << "\n"
            << "========================================================\n"
            << "Service Name      : " << service_name << "\n"
            << "Trigger Timestamp : " << formatTimestamp(start_time_ms) << "\n"
            << "Initial Diagnosis : " << faultTypeToString(initial_fault) << "\n"
            << "Pre-Action PID    : " << snapshot.target_pid << "\n"
            << "Pre-Action RSS    : " << (snapshot.metrics.rss_kb / 1024) << " MB\n"
            << "Recovery Action   : " << recoveryActionToString(action_taken) << "\n"
            << "Retry Count       : " << retry_count << "\n"
            << "Final Outcome     : " << recoveryStatusToString(status) << "\n";
        if (verification.is_successful) {
            oss << "New Process PID   : " << verification.new_pid << "\n"
                << "Recovery Latency  : " << std::fixed << std::setprecision(2) 
                << ((end_time_ms - start_time_ms) / 1000.0) << " seconds\n";
        }
        oss << "Verification Log  : " << verification.verification_notes << "\n"
            << "========================================================\n";
        return oss.str();
    }
};

} // namespace sentinel

#endif // LINUX_SENTINEL_COMMON_H
