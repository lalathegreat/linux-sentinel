#include "Common.h"
#include "HealthCollector.h"
#include "CheckpointManager.h"
#include "VerificationEngine.h"
#include "RecoveryManager.h"
#include "Logger.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>

using namespace sentinel;

void testAllowlistRejection() {
    auto collector = std::make_shared<HealthCollector>();
    auto cp_mgr = std::make_shared<CheckpointManager>("logs/snapshots");
    auto verifier = std::make_shared<VerificationEngine>(collector);
    RecoveryManager rm(collector, cp_mgr, verifier);

    DiagnosisRecord diag;
    diag.service_name = "unregistered_malicious_proc";
    diag.recommended_action = RecoveryActionType::ACTION_FORCEFUL_RESTART;
    diag.diagnosed_fault = FaultType::HIGH_MEMORY;
    diag.root_cause_evidence = "Test";

    ProcessMetrics metrics;
    metrics.pid = 99999;
    metrics.service_name = "unregistered_malicious_proc";

    RecoveryResult res = rm.executeRecovery(diag, metrics);
    assert(!res.success);
    assert(res.status == RecoveryStatus::STATUS_FAILED);
    std::cout << "[PASS] Strict allowlist rejection verified." << std::endl;
}

void testCircuitBreaker() {
    auto collector = std::make_shared<HealthCollector>();
    auto cp_mgr = std::make_shared<CheckpointManager>("logs/snapshots");
    auto verifier = std::make_shared<VerificationEngine>(collector);
    
    // Max 3 retries within 60s
    RecoveryManager rm(collector, cp_mgr, verifier, 500, 3, 60000);

    // Register service that crashes immediately on start
    rm.registerService("flapping_service", "./fault_app", {"--crash-on-start"});

    DiagnosisRecord diag;
    diag.service_name = "flapping_service";
    diag.recommended_action = RecoveryActionType::ACTION_GRACEFUL_RESTART;
    diag.diagnosed_fault = FaultType::PROCESS_TERMINATED;
    diag.root_cause_evidence = "Process died on startup";

    ProcessMetrics metrics;
    metrics.pid = 12345;
    metrics.service_name = "flapping_service";

    // Attempt 1
    RecoveryResult r1 = rm.executeRecovery(diag, metrics);
    assert(r1.retry_count == 1);

    // Attempt 2
    RecoveryResult r2 = rm.executeRecovery(diag, metrics);
    assert(r2.retry_count == 2);

    // Attempt 3
    RecoveryResult r3 = rm.executeRecovery(diag, metrics);
    assert(r3.retry_count == 3);

    // Attempt 4 -> Circuit Breaker MUST TRIP
    RecoveryResult r4 = rm.executeRecovery(diag, metrics);
    assert(r4.status == RecoveryStatus::STATUS_CIRCUIT_BREAKER_TRIPPED);
    assert(!r4.success);
    std::cout << "[PASS] Circuit Breaker tripped after 3 retries (Safe Mode engaged)." << std::endl;
}

int main() {
    auto& logger = Logger::getInstance();
    logger.init("logs/test_recovery.log", false);

    std::cout << "Running Recovery & Circuit Breaker Unit Tests...\n";
    testAllowlistRejection();
    testCircuitBreaker();
    std::cout << "All Recovery Subsystem Tests Passed Successfully!\n";
    return 0;
}
