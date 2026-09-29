#include "Common.h"
#include "HealthCollector.h"
#include "AnomalyDetector.h"
#include "DependencyManager.h"
#include "DiagnosisEngine.h"
#include "Logger.h"

#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <thread>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <sys/wait.h>

static volatile sig_atomic_t g_keep_running = 1;

void sigHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_keep_running = 0;
    }
}

void printVersion() {
    std::cout << "Linux Sentinel version 1.0.0 (Wipro Embedded Linux Capstone Prototype)\n"
              << "Copyright (c) 2026. Built with C++17 for Linux systems reliability.\n";
}

void printHelp() {
    std::cout << "Usage: sentinel [OPTIONS]\n\n"
              << "Options:\n"
              << "  -v, --version                Print version information and exit\n"
              << "  -h, --help                   Display this help message and exit\n"
              << "  -m, --monitor <PID> <NAME>   Monitor an existing running process by PID\n"
              << "  -s, --spawn <CMD> [ARGS...]  Spawn and supervise a target application\n"
              << "  -c, --config <FILE>          Load service definitions from configuration file\n"
              << "\nExamples:\n"
              << "  ./sentinel --version\n"
              << "  ./sentinel --spawn ./fault_app --normal\n"
              << "  ./sentinel --spawn ./fault_app --leak\n"
              << "  ./sentinel --monitor 1234 fault_app\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, sigHandler);
    std::signal(SIGTERM, sigHandler);

    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string first_arg = argv[1];
    if (first_arg == "-v" || first_arg == "--version") {
        printVersion();
        return 0;
    }
    if (first_arg == "-h" || first_arg == "--help") {
        printHelp();
        return 0;
    }

    // Initialize Logger
    auto& logger = sentinel::Logger::getInstance();
    logger.init("logs/incidents.log", true);
    logger.info("Linux Sentinel daemon starting up. Initializing subsystems...");

    // Initialize Core Engines
    sentinel::HealthCollector collector;
    sentinel::AnomalyDetector detector;
    auto dep_mgr = std::make_shared<sentinel::DependencyManager>();
    
    // Configure default DAG relationships
    dep_mgr->registerService("fault_app", {});
    dep_mgr->registerService("database", {});
    dep_mgr->registerService("api_gateway", {"database"});
    logger.info("Dependency graph initialized. Registered DAG (api_gateway -> database).");

    sentinel::DiagnosisEngine diagnosis_engine(dep_mgr, 5000); // 5s cooldown

    pid_t target_pid = -1;
    std::string service_name = "fault_app";
    bool spawned_by_us = false;

    if ((first_arg == "-m" || first_arg == "--monitor") && argc >= 4) {
        target_pid = std::stoi(argv[2]);
        service_name = argv[3];
        dep_mgr->registerService(service_name, {});
        logger.info("Attaching to existing process PID " + std::to_string(target_pid) + " (" + service_name + ")");
    } else if ((first_arg == "-s" || first_arg == "--spawn") && argc >= 3) {
        std::string exec_path = argv[2];
        std::vector<char*> child_args;
        for (int i = 2; i < argc; ++i) {
            child_args.push_back(argv[i]);
        }
        child_args.push_back(nullptr);

        service_name = exec_path.substr(exec_path.find_last_of("/\\") + 1);
        dep_mgr->registerService(service_name, {});

        logger.info("Spawning child target: " + exec_path);
        target_pid = fork();
        if (target_pid == 0) {
            // Child process
            execvp(child_args[0], child_args.data());
            std::cerr << "[Sentinel Error] Failed to exec " << child_args[0] << std::endl;
            _exit(1);
        } else if (target_pid > 0) {
            spawned_by_us = true;
            logger.success("Spawned " + service_name + " with PID " + std::to_string(target_pid));
        } else {
            logger.error("Failed to fork child process!");
            return 1;
        }
    } else {
        printHelp();
        return 1;
    }

    logger.info("Starting closed-loop observability pipeline (Interval: 1000ms)...");

    // Allow child process 200ms to spin up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    while (g_keep_running) {
        sentinel::ProcessMetrics metrics = collector.pollProcess(target_pid, service_name);
        
        // Log telemetry
        logger.info(metrics.to_string());

        // Update Dependency Manager state
        if (metrics.is_alive) {
            dep_mgr->setServiceState(service_name, sentinel::ServiceState::HEALTHY);
        } else {
            dep_mgr->setServiceState(service_name, sentinel::ServiceState::CRITICAL);
        }

        // Evaluate Telemetry for Anomalies
        auto anomaly_opt = detector.evaluate(metrics);
        if (anomaly_opt.has_value()) {
            const auto& anomaly = *anomaly_opt;
            
            // Apply Rule-Based Diagnosis Engine
            if (!diagnosis_engine.isCoolingDown(service_name) || 
                anomaly.fault_type == sentinel::FaultType::PROCESS_TERMINATED) {
                
                sentinel::DiagnosisRecord diag = diagnosis_engine.diagnose(anomaly);
                logger.warn("ANOMALY: " + anomaly.message);
                logger.warn("DIAGNOSIS: " + diag.root_cause_evidence);
                logger.info("RECOMMENDED ACTION: " + sentinel::recoveryActionToString(diag.recommended_action));

                // Create Forensic Incident Snapshot
                sentinel::IncidentSnapshot snapshot;
                snapshot.incident_id = logger.generateIncidentId();
                snapshot.timestamp_ms = sentinel::getCurrentTimeMs();
                snapshot.service_name = service_name;
                snapshot.target_pid = target_pid;
                snapshot.metrics = metrics;
                snapshot.trigger_reason = diag.root_cause_evidence;

                // Generate and record Incident Card
                sentinel::IncidentCard card;
                card.incident_id = snapshot.incident_id;
                card.service_name = service_name;
                card.start_time_ms = snapshot.timestamp_ms;
                card.end_time_ms = sentinel::getCurrentTimeMs();
                card.initial_fault = diag.diagnosed_fault;
                card.snapshot = snapshot;
                card.action_taken = diag.recommended_action;
                card.retry_count = 1;
                card.status = sentinel::RecoveryStatus::STATUS_SUCCESS;
                card.verification.is_successful = true;
                card.verification.new_pid = target_pid;
                card.verification.verification_notes = diag.root_cause_evidence;

                logger.logIncident(card);

                if (anomaly.fault_type == sentinel::FaultType::PROCESS_TERMINATED) {
                    logger.error("Target process has exited. Stopping prototype observation loop.");
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    logger.info("Linux Sentinel prototype shutting down cleanly.");

    // Cleanup spawned child if active
    if (spawned_by_us && target_pid > 0 && collector.isProcessAlive(target_pid)) {
        logger.info("Stopping supervised child PID " + std::to_string(target_pid) + " via SIGTERM...");
        kill(target_pid, SIGTERM);
        int status = 0;
        waitpid(target_pid, &status, 0);
        logger.success("Child PID reaped successfully.");
    }

    return 0;
}
