#include "VerificationEngine.h"
#include <thread>
#include <chrono>

namespace sentinel {

VerificationEngine::VerificationEngine(std::shared_ptr<HealthCollector> collector)
    : collector_(std::move(collector)) {}

VerificationResult VerificationEngine::verifyProcess(int new_pid, 
                                                     const std::string& service_name, 
                                                     long expected_max_rss_kb, 
                                                     uint32_t stabilization_ms) {
    VerificationResult result;
    result.new_pid = new_pid;
    result.timestamp_ms = getCurrentTimeMs();
    result.is_successful = false;

    if (new_pid <= 0) {
        result.verification_notes = "Verification failed: Invalid new PID assigned.";
        return result;
    }

    uint32_t elapsed_ms = 0;
    const uint32_t step_ms = 400;

    while (elapsed_ms < stabilization_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
        elapsed_ms += step_ms;

        // 1. Check liveness
        if (!collector_->isProcessAlive(new_pid)) {
            result.verification_notes = "Verification failed: New PID " + std::to_string(new_pid) + 
                                       " died during the stabilization window (" + 
                                       std::to_string(elapsed_ms) + " ms elapsed).";
            result.stabilization_time_sec = elapsed_ms / 1000.0;
            return result;
        }

        // 2. Poll telemetry
        ProcessMetrics metrics = collector_->pollProcess(new_pid, service_name);
        
        // Check for zombie state
        if (metrics.state_char == 'Z') {
            result.verification_notes = "Verification failed: New process entered Zombie state.";
            result.stabilization_time_sec = elapsed_ms / 1000.0;
            return result;
        }

        // Check if memory is within threshold
        if (expected_max_rss_kb > 0 && metrics.rss_kb > expected_max_rss_kb) {
            result.verification_notes = "Verification failed: Post-restart memory (" + 
                                       std::to_string(metrics.rss_kb / 1024) + 
                                       " MB) exceeded threshold immediately.";
            result.stabilization_time_sec = elapsed_ms / 1000.0;
            return result;
        }
    }

    // Process survived stabilization window in a healthy state
    result.is_successful = true;
    result.stabilization_time_sec = elapsed_ms / 1000.0;
    result.verification_notes = "Process PID " + std::to_string(new_pid) + 
                               " successfully stabilized across " + 
                               std::to_string(result.stabilization_time_sec) + 
                               "s window (State: Active, Memory: Normal).";
    return result;
}

} // namespace sentinel
