#ifndef LINUX_SENTINEL_RECOVERY_MANAGER_H
#define LINUX_SENTINEL_RECOVERY_MANAGER_H

#include "Common.h"
#include "HealthCollector.h"
#include "CheckpointManager.h"
#include "VerificationEngine.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <memory>
#include <mutex>

namespace sentinel {

struct ManagedService {
    std::string name;
    std::string executable;
    std::vector<std::string> args;
    bool allow_recovery{true};
    int current_pid{-1};
    ServiceState state{ServiceState::HEALTHY};
};

struct RecoveryResult {
    bool success{false};
    std::string service_name;
    int old_pid{-1};
    int new_pid{-1};
    RecoveryActionType action_taken{RecoveryActionType::ACTION_NONE};
    RecoveryStatus status{RecoveryStatus::STATUS_PENDING};
    int retry_count{0};
    double total_recovery_time_sec{0.0};
    std::string summary;
};

class RecoveryManager {
public:
    RecoveryManager(std::shared_ptr<HealthCollector> collector,
                    std::shared_ptr<CheckpointManager> checkpoint_mgr,
                    std::shared_ptr<VerificationEngine> verification_engine,
                    uint32_t graceful_timeout_ms = 3000,
                    int max_retries = 3,
                    uint64_t circuit_breaker_window_ms = 60000);
    ~RecoveryManager() = default;

    // Register a service on the strict allowlist
    bool registerService(const std::string& name, 
                         const std::string& executable, 
                         const std::vector<std::string>& args, 
                         bool allow_recovery = true);

    void updateActivePid(const std::string& name, int pid);
    int getActivePid(const std::string& name) const;
    bool isAllowlisted(const std::string& name) const;
    bool isCircuitBreakerTripped(const std::string& name);
    void resetCircuitBreaker(const std::string& name);

    // Execute the complete closed-loop recovery sequence
    RecoveryResult executeRecovery(const DiagnosisRecord& diagnosis, 
                                   const ProcessMetrics& current_metrics);

private:
    std::shared_ptr<HealthCollector> collector_;
    std::shared_ptr<CheckpointManager> checkpoint_mgr_;
    std::shared_ptr<VerificationEngine> verification_engine_;

    uint32_t graceful_timeout_ms_{3000};
    int max_retries_{3};
    uint64_t circuit_breaker_window_ms_{60000};

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ManagedService> services_;
    std::unordered_map<std::string, std::deque<uint64_t>> restart_history_;

    // Internal execution steps
    bool stopProcess(int pid, RecoveryActionType action);
    int spawnProcess(const ManagedService& service);
};

} // namespace sentinel

#endif // LINUX_SENTINEL_RECOVERY_MANAGER_H
