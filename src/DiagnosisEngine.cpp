#include "DiagnosisEngine.h"

namespace sentinel {

DiagnosisEngine::DiagnosisEngine(std::shared_ptr<DependencyManager> dep_mgr, 
                                 uint64_t cooldown_period_ms)
    : dep_mgr_(std::move(dep_mgr)), cooldown_period_ms_(cooldown_period_ms) {}

bool DiagnosisEngine::isCoolingDown(const std::string& service_name) const {
    auto it = last_diagnosis_timestamps_.find(service_name);
    if (it != last_diagnosis_timestamps_.end()) {
        uint64_t now_ms = getCurrentTimeMs();
        if (now_ms - it->second < cooldown_period_ms_) {
            return true;
        }
    }
    return false;
}

void DiagnosisEngine::resetCooldown(const std::string& service_name) {
    last_diagnosis_timestamps_.erase(service_name);
}

DiagnosisRecord DiagnosisEngine::diagnose(const AnomalyReport& anomaly) {
    DiagnosisRecord record;
    record.service_name = anomaly.service_name;
    record.timestamp_ms = getCurrentTimeMs();

    // 1. Check for immediate process termination (highest priority)
    if (anomaly.fault_type == FaultType::PROCESS_TERMINATED) {
        record.diagnosed_fault = FaultType::PROCESS_TERMINATED;
        record.severity = Severity::SEV_CRITICAL;
        record.root_cause_evidence = "Process disappeared from /proc table unexpectedly (Crash, SIGKILL, or Exit).";
        record.recommended_action = RecoveryActionType::ACTION_GRACEFUL_RESTART;
        last_diagnosis_timestamps_[anomaly.service_name] = record.timestamp_ms;
        return record;
    }

    // 2. Correlate with upstream dependencies (Distinguish symptom from root cause)
    if (dep_mgr_) {
        std::string failing_dep;
        if (!dep_mgr_->areDependenciesHealthy(anomaly.service_name, failing_dep)) {
            record.diagnosed_fault = FaultType::DEPENDENCY_FAILURE;
            record.severity = Severity::SEV_WARNING;
            record.blocked_by_dependency = failing_dep;
            record.root_cause_evidence = "Observed degradation is a symptom caused by upstream outage: " + failing_dep;
            record.recommended_action = RecoveryActionType::ACTION_NONE; // Do NOT restart downstream service!
            last_diagnosis_timestamps_[anomaly.service_name] = record.timestamp_ms;
            return record;
        }
    }

    // 3. Classify based on telemetry trends and patterns
    switch (anomaly.fault_type) {
        case FaultType::MEMORY_GROWTH:
            record.diagnosed_fault = FaultType::MEMORY_GROWTH;
            record.severity = Severity::SEV_WARNING;
            record.root_cause_evidence = "Monotonic memory allocation slope without deallocation (Systemic Memory Leak).";
            record.recommended_action = RecoveryActionType::ACTION_GRACEFUL_RESTART;
            break;

        case FaultType::HIGH_MEMORY:
            record.diagnosed_fault = FaultType::HIGH_MEMORY;
            record.severity = Severity::SEV_CRITICAL;
            record.root_cause_evidence = "Process exceeded maximum configured RSS memory threshold; imminent kernel OOM termination.";
            record.recommended_action = RecoveryActionType::ACTION_FORCEFUL_RESTART;
            break;

        case FaultType::CPU_SATURATION:
            record.diagnosed_fault = FaultType::CPU_SATURATION;
            record.severity = Severity::SEV_WARNING;
            record.root_cause_evidence = "Sustained high CPU usage across multiple sample windows (Thread Spin-Lock or Workload Saturation).";
            record.recommended_action = RecoveryActionType::ACTION_GRACEFUL_RESTART;
            break;

        default:
            record.diagnosed_fault = anomaly.fault_type;
            record.severity = anomaly.severity;
            record.root_cause_evidence = anomaly.message;
            record.recommended_action = RecoveryActionType::ACTION_NONE;
            break;
    }

    last_diagnosis_timestamps_[anomaly.service_name] = record.timestamp_ms;
    return record;
}

} // namespace sentinel
