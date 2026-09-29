#include "RecoveryManager.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

namespace sentinel {

RecoveryManager::RecoveryManager(std::shared_ptr<HealthCollector> collector,
                                 std::shared_ptr<CheckpointManager> checkpoint_mgr,
                                 std::shared_ptr<VerificationEngine> verification_engine,
                                 uint32_t graceful_timeout_ms,
                                 int max_retries,
                                 uint64_t circuit_breaker_window_ms)
    : collector_(std::move(collector)),
      checkpoint_mgr_(std::move(checkpoint_mgr)),
      verification_engine_(std::move(verification_engine)),
      graceful_timeout_ms_(graceful_timeout_ms),
      max_retries_(max_retries),
      circuit_breaker_window_ms_(circuit_breaker_window_ms) {}

bool RecoveryManager::registerService(const std::string& name, 
                                     const std::string& executable, 
                                     const std::vector<std::string>& args, 
                                     bool allow_recovery) {
    std::lock_guard<std::mutex> lock(mutex_);
    ManagedService s;
    s.name = name;
    s.executable = executable;
    s.args = args;
    s.allow_recovery = allow_recovery;
    s.current_pid = -1;
    s.state = ServiceState::HEALTHY;
    services_[name] = s;
    return true;
}

void RecoveryManager::updateActivePid(const std::string& name, int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        it->second.current_pid = pid;
    }
}

int RecoveryManager::getActivePid(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second.current_pid;
    }
    return -1;
}

bool RecoveryManager::isAllowlisted(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (services_.find(name) != services_.end());
}

bool RecoveryManager::isCircuitBreakerTripped(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now_ms = getCurrentTimeMs();
    auto& history = restart_history_[name];

    // Prune entries older than the sliding window
    while (!history.empty() && (now_ms - history.front() > circuit_breaker_window_ms_)) {
        history.pop_front();
    }

    return (static_cast<int>(history.size()) >= max_retries_);
}

void RecoveryManager::resetCircuitBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    restart_history_[name].clear();
    auto it = services_.find(name);
    if (it != services_.end()) {
        it->second.state = ServiceState::HEALTHY;
    }
}

bool RecoveryManager::stopProcess(int pid, RecoveryActionType action) {
    if (pid <= 0 || !collector_->isProcessAlive(pid)) {
        // Already dead, reap any zombie
        int status = 0;
        waitpid(pid, &status, WNOHANG);
        return true;
    }

    if (action == RecoveryActionType::ACTION_GRACEFUL_RESTART) {
        // Tier 1: Send SIGTERM
        kill(pid, SIGTERM);

        uint32_t elapsed_ms = 0;
        const uint32_t step_ms = 100;
        while (elapsed_ms < graceful_timeout_ms_) {
            int status = 0;
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid || !collector_->isProcessAlive(pid)) {
                return true; // Graceful exit achieved
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
            elapsed_ms += step_ms;
        }

        // Tier 2: Timeout expired, escalate to SIGKILL
        kill(pid, SIGKILL);
        int status = 0;
        waitpid(pid, &status, 0);
        return true;
    } else if (action == RecoveryActionType::ACTION_FORCEFUL_RESTART) {
        // Immediate SIGKILL
        kill(pid, SIGKILL);
        int status = 0;
        waitpid(pid, &status, 0);
        return true;
    }

    return false;
}

int RecoveryManager::spawnProcess(const ManagedService& service) {
    std::vector<char*> argv_ptrs;
    argv_ptrs.push_back(const_cast<char*>(service.executable.c_str()));
    for (const auto& arg : service.args) {
        argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
    }
    argv_ptrs.push_back(nullptr);

    pid_t new_pid = fork();
    if (new_pid == 0) {
        // In Child Process: Execute target binary
        execvp(argv_ptrs[0], argv_ptrs.data());
        std::cerr << "[RecoveryManager Error] Failed to exec " << argv_ptrs[0] << std::endl;
        _exit(1);
    } else if (new_pid > 0) {
        return new_pid;
    } else {
        return -1;
    }
}

RecoveryResult RecoveryManager::executeRecovery(const DiagnosisRecord& diagnosis, 
                                               const ProcessMetrics& current_metrics) {
    uint64_t start_ms = getCurrentTimeMs();
    RecoveryResult result;
    result.service_name = diagnosis.service_name;
    result.action_taken = diagnosis.recommended_action;
    result.old_pid = current_metrics.pid;
    result.success = false;
    result.status = RecoveryStatus::STATUS_PENDING;

    auto& logger = Logger::getInstance();
    std::string incident_id = logger.generateIncidentId();

    // 1. Allowlist and Permission Check
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = services_.find(diagnosis.service_name);
    if (it == services_.end() || !it->second.allow_recovery) {
        result.status = RecoveryStatus::STATUS_FAILED;
        result.summary = "Recovery aborted: Service not on strict allowlist or recovery disabled.";
        logger.error(result.summary);
        return result;
    }
    ManagedService service = it->second;

    // 2. Circuit Breaker Enforcement
    uint64_t now_ms = getCurrentTimeMs();
    auto& history = restart_history_[service.name];
    while (!history.empty() && (now_ms - history.front() > circuit_breaker_window_ms_)) {
        history.pop_front();
    }

    if (static_cast<int>(history.size()) >= max_retries_) {
        it->second.state = ServiceState::SAFE_MODE;
        result.status = RecoveryStatus::STATUS_CIRCUIT_BREAKER_TRIPPED;
        result.retry_count = static_cast<int>(history.size());
        result.summary = "CIRCUIT BREAKER TRIPPED: Exceeded " + std::to_string(max_retries_) + 
                         " restarts in " + std::to_string(circuit_breaker_window_ms_ / 1000) + 
                         "s. Service placed into SAFE_MODE.";
        logger.error(result.summary);

        // Record Circuit Breaker Incident Card
        IncidentCard cb_card;
        cb_card.incident_id = incident_id;
        cb_card.service_name = service.name;
        cb_card.start_time_ms = start_ms;
        cb_card.end_time_ms = getCurrentTimeMs();
        cb_card.initial_fault = diagnosis.diagnosed_fault;
        cb_card.action_taken = RecoveryActionType::ACTION_ENTER_SAFE_MODE;
        cb_card.retry_count = result.retry_count;
        cb_card.status = RecoveryStatus::STATUS_CIRCUIT_BREAKER_TRIPPED;
        cb_card.verification.is_successful = false;
        cb_card.verification.verification_notes = result.summary;
        logger.logIncident(cb_card);

        return result;
    }

    // Record this attempt
    history.push_back(now_ms);
    result.retry_count = static_cast<int>(history.size());
    lock.unlock();

    logger.info("Executing recovery for " + service.name + " (Attempt " + 
                std::to_string(result.retry_count) + " of " + std::to_string(max_retries_) + ")...");

    // 3. Pre-Recovery Forensic Incident Snapshot
    IncidentSnapshot snapshot = checkpoint_mgr_->captureSnapshot(
        incident_id, service.name, current_metrics.pid, current_metrics, diagnosis.root_cause_evidence);
    logger.info("Forensic checkpoint captured: " + incident_id);

    // 4. Signal Escalation & Termination
    logger.info("Terminating degraded process PID " + std::to_string(current_metrics.pid) + "...");
    stopProcess(current_metrics.pid, diagnosis.recommended_action);

    // 5. Respawn Target Service
    logger.info("Respawning service " + service.name + " (" + service.executable + ")...");
    int new_pid = spawnProcess(service);
    if (new_pid <= 0) {
        result.status = RecoveryStatus::STATUS_FAILED;
        result.summary = "Failed to fork and execute new process!";
        logger.error(result.summary);
        return result;
    }
    result.new_pid = new_pid;
    updateActivePid(service.name, new_pid);
    logger.success("New process instance spawned with PID " + std::to_string(new_pid));

    // 6. Post-Recovery Health Verification
    logger.info("Initiating post-recovery verification (Stabilization window: 2000ms)...");
    VerificationResult v_res = verification_engine_->verifyProcess(new_pid, service.name);

    result.total_recovery_time_sec = (getCurrentTimeMs() - start_ms) / 1000.0;
    result.success = v_res.is_successful;
    result.status = v_res.is_successful ? RecoveryStatus::STATUS_SUCCESS : RecoveryStatus::STATUS_FAILED;
    result.summary = v_res.verification_notes;

    if (v_res.is_successful) {
        logger.success("RECOVERY VERIFIED SUCCESSFUL in " + 
                       std::to_string(result.total_recovery_time_sec) + "s.");
    } else {
        logger.error("RECOVERY VERIFICATION FAILED: " + v_res.verification_notes);
    }

    // 7. Audit Logging (Incident Card)
    IncidentCard card;
    card.incident_id = incident_id;
    card.service_name = service.name;
    card.start_time_ms = start_ms;
    card.end_time_ms = getCurrentTimeMs();
    card.initial_fault = diagnosis.diagnosed_fault;
    card.snapshot = snapshot;
    card.action_taken = diagnosis.recommended_action;
    card.retry_count = result.retry_count;
    card.status = result.status;
    card.verification = v_res;
    logger.logIncident(card);

    return result;
}

} // namespace sentinel
