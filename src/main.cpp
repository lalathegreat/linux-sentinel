#include "Common.h"
#include "HealthCollector.h"
#include "AnomalyDetector.h"
#include "DependencyManager.h"
#include "DiagnosisEngine.h"
#include "CheckpointManager.h"
#include "VerificationEngine.h"
#include "RecoveryManager.h"
#include "ConfigManager.h"
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
              << "  -c, --config <FILE>          Load service definitions from configuration file\n"
              << "  -m, --monitor <PID> <NAME>   Monitor an existing running process by PID\n"
              << "  -s, --spawn <CMD> [ARGS...]  Spawn and supervise a target application\n"
              << "  -d, --duration <SEC>         Run monitoring loop for N seconds, then exit\n"
              << "\nExamples:\n"
              << "  ./sentinel --version\n"
              << "  ./sentinel --config config/sentinel.conf\n"
              << "  ./sentinel --spawn ./build/fault_app --normal\n"
              << "  ./sentinel --spawn ./build/fault_app --leak\n"
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

    std::string config_path;
    std::string mode;
    pid_t target_pid = -1;
    std::string service_name = "fault_app";
    std::string exec_path = "./build/fault_app";
    std::vector<std::string> child_args_str;
    int max_duration_sec = -1;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_path = argv[++i];
            } else {
                std::cerr << "[Sentinel Error] Option --config requires a file argument." << std::endl;
                return 1;
            }
        } else if (arg == "-m" || arg == "--monitor") {
            mode = "monitor";
            if (i + 2 < argc) {
                try {
                    target_pid = std::stoi(argv[++i]);
                } catch (...) {
                    std::cerr << "[Sentinel Error] Invalid PID provided for --monitor: " << argv[i] << std::endl;
                    return 1;
                }
                service_name = argv[++i];
            } else {
                std::cerr << "[Sentinel Error] Option --monitor requires <PID> and <NAME>." << std::endl;
                return 1;
            }
        } else if (arg == "-s" || arg == "--spawn") {
            mode = "spawn";
            if (i + 1 < argc) {
                exec_path = argv[++i];
                while (i + 1 < argc) {
                    std::string next_arg = argv[i + 1];
                    if (next_arg == "-d" || next_arg == "--duration" || 
                        next_arg == "-c" || next_arg == "--config") {
                        break;
                    }
                    child_args_str.push_back(argv[++i]);
                }
            } else {
                std::cerr << "[Sentinel Error] Option --spawn requires executable path." << std::endl;
                return 1;
            }
        } else if (arg == "-d" || arg == "--duration") {
            if (i + 1 < argc) {
                max_duration_sec = std::stoi(argv[++i]);
            }
        }
    }

    sentinel::ConfigManager config_mgr;
    if (!config_path.empty()) {
        if (!config_mgr.loadFromFile(config_path)) {
            std::cerr << "[Sentinel Error] Malformed configuration file '" << config_path 
                      << "': " << config_mgr.getLastError() << std::endl;
            return 1;
        }
        std::cout << "[Sentinel Info] Loaded and validated configuration from '" << config_path << "' successfully." << std::endl;
        
        // If no explicit spawn/monitor mode provided, check config services
        if (mode.empty()) {
            const auto& services = config_mgr.getServices();
            if (!services.empty()) {
                auto it = services.find("fault_app");
                const auto& first_svc = (it != services.end()) ? it->second : services.begin()->second;
                mode = "spawn";
                service_name = first_svc.name;
                exec_path = first_svc.executable;
                child_args_str = first_svc.arguments;
            }
        }
    }

    if (mode.empty()) {
        if (!config_path.empty()) {
            // Configuration validated cleanly, exit 0 if only config check
            return 0;
        }
        printHelp();
        return 1;
    }

    // Initialize Logger
    auto& logger = sentinel::Logger::getInstance();
    std::string log_file = config_path.empty() ? "logs/incidents.log" : config_mgr.getGlobalConfig().log_file_path;
    bool enable_color = config_path.empty() ? true : config_mgr.getGlobalConfig().enable_ansi_tui;
    logger.init(log_file, enable_color);
    logger.info("Linux Sentinel daemon starting up. Initializing subsystems...");

    // Initialize Core Subsystems
    auto collector = std::make_shared<sentinel::HealthCollector>();
    
    // Anomaly detector thresholds
    sentinel::DetectionThresholds thresholds;
    if (!config_path.empty()) {
        const auto& dcfg = config_mgr.getDetectionConfig();
        thresholds.consecutive_breaches_required = static_cast<int>(dcfg.consecutive_samples_required);
        thresholds.memory_leak_slope_kb = static_cast<long>(dcfg.memory_leak_slope_threshold_kb);
        thresholds.max_rss_limit_kb = static_cast<long>(dcfg.memory_max_rss_limit_kb);
        thresholds.cpu_saturation_percent = dcfg.cpu_saturation_threshold_percent;
        thresholds.window_size = dcfg.history_window_size;
    }
    auto detector = std::make_shared<sentinel::AnomalyDetector>(thresholds);

    auto dep_mgr = std::make_shared<sentinel::DependencyManager>();
    auto cp_mgr = std::make_shared<sentinel::CheckpointManager>("logs/snapshots");
    auto verifier = std::make_shared<sentinel::VerificationEngine>(collector);

    // Recovery manager parameters
    uint32_t stop_timeout = config_path.empty() ? 3000 : config_mgr.getRecoveryConfig().graceful_stop_timeout_ms;
    uint32_t max_retries = config_path.empty() ? 3 : config_mgr.getRecoveryConfig().circuit_breaker_max_retries;
    uint32_t breaker_window = config_path.empty() ? 60000 : config_mgr.getRecoveryConfig().circuit_breaker_window_ms;
    auto recovery_mgr = std::make_shared<sentinel::RecoveryManager>(collector, cp_mgr, verifier, stop_timeout, max_retries, breaker_window);

    // Register Default DAG Relationships or config services
    if (!config_path.empty()) {
        for (const auto& [name, svc] : config_mgr.getServices()) {
            dep_mgr->registerService(name, svc.depends_on);
            recovery_mgr->registerService(name, svc.executable, svc.arguments, svc.allow_auto_recovery);
        }
    } else {
        dep_mgr->registerService("fault_app", {});
        dep_mgr->registerService("database", {});
        dep_mgr->registerService("api_gateway", {"database"});
    }
    logger.info("Dependency graph initialized. Registered DAG services.");

    uint32_t diag_cooldown = config_path.empty() ? 5000 : config_mgr.getRecoveryConfig().diagnosis_cooldown_ms;
    sentinel::DiagnosisEngine diagnosis_engine(dep_mgr, diag_cooldown);

    bool spawned_by_us = false;

    if (mode == "monitor") {
        // T2 requirement: verify target is actually running at startup
        if (!collector->isProcessAlive(target_pid)) {
            std::cerr << "[Sentinel Error] Target process PID " << target_pid 
                      << " is not running at startup. No unrelated process touched." << std::endl;
            return 1;
        }

        dep_mgr->registerService(service_name, {});
        recovery_mgr->registerService(service_name, exec_path, child_args_str, true);
        recovery_mgr->updateActivePid(service_name, target_pid);
        logger.info("Attaching to existing process PID " + std::to_string(target_pid) + " (" + service_name + ")");
    } else if (mode == "spawn") {
        if (service_name.empty()) {
            service_name = exec_path.substr(exec_path.find_last_of("/\\") + 1);
        }
        dep_mgr->registerService(service_name, {});
        recovery_mgr->registerService(service_name, exec_path, child_args_str, true);

        // Initial launch
        logger.info("Spawning child target: " + exec_path);
        target_pid = fork();
        if (target_pid == 0) {
            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>(exec_path.c_str()));
            for (const auto& a : child_args_str) {
                c_args.push_back(const_cast<char*>(a.c_str()));
            }
            c_args.push_back(nullptr);
            execvp(c_args[0], c_args.data());
            std::cerr << "[Sentinel Error] Failed to exec " << c_args[0] << std::endl;
            _exit(1);
        } else if (target_pid > 0) {
            spawned_by_us = true;
            recovery_mgr->updateActivePid(service_name, target_pid);
            logger.success("Spawned " + service_name + " with PID " + std::to_string(target_pid));
        } else {
            logger.error("Failed to fork child process!");
            return 1;
        }
    }

    uint32_t polling_interval_ms = config_path.empty() ? 1000 : config_mgr.getGlobalConfig().polling_interval_ms;
    logger.info("Starting closed-loop self-healing engine (Interval: " + std::to_string(polling_interval_ms) + "ms)...");

    // Allow child process 200ms to spin up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto start_time = std::chrono::steady_clock::now();

    while (g_keep_running) {
        if (max_duration_sec > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (elapsed >= max_duration_sec) {
                logger.info("Reached maximum duration (" + std::to_string(max_duration_sec) + "s). Exiting loop.");
                break;
            }
        }

        sentinel::ProcessMetrics metrics = collector->pollProcess(target_pid, service_name);
        
        // Log telemetry
        logger.info(metrics.to_string());

        // Update Dependency Manager state
        if (metrics.is_alive) {
            dep_mgr->setServiceState(service_name, sentinel::ServiceState::HEALTHY);
        } else {
            dep_mgr->setServiceState(service_name, sentinel::ServiceState::CRITICAL);
        }

        // Evaluate Telemetry for Anomalies
        auto anomaly_opt = detector->evaluate(metrics);
        if (anomaly_opt.has_value()) {
            const auto& anomaly = *anomaly_opt;
            
            // Apply Rule-Based Diagnosis Engine
            if (!diagnosis_engine.isCoolingDown(service_name) || 
                anomaly.fault_type == sentinel::FaultType::PROCESS_TERMINATED) {
                
                sentinel::DiagnosisRecord diag = diagnosis_engine.diagnose(anomaly);
                logger.warn("ANOMALY: " + anomaly.message);
                logger.warn("DIAGNOSIS: " + diag.root_cause_evidence);

                // Check if Recovery Action is recommended
                if (diag.recommended_action != sentinel::RecoveryActionType::ACTION_NONE) {
                    logger.info("INITIATING SELF-HEALING RECOVERY SEQUENCE...");

                    sentinel::RecoveryResult rec_res = recovery_mgr->executeRecovery(diag, metrics);
                    if (rec_res.success) {
                        logger.success("SELF-HEALING COMPLETE: Service " + service_name + 
                                       " recovered with new PID " + std::to_string(rec_res.new_pid));
                        target_pid = rec_res.new_pid;
                        detector->resetHistory(service_name);
                    } else if (rec_res.status == sentinel::RecoveryStatus::STATUS_CIRCUIT_BREAKER_TRIPPED) {
                        logger.error("CRITICAL: Circuit breaker tripped! Halting auto-recovery for " + service_name);
                        break;
                    } else {
                        logger.error("Self-healing recovery failed: " + rec_res.summary);
                    }
                } else if (!diag.blocked_by_dependency.empty()) {
                    logger.warn("Recovery withheld: Upstream dependency is down (" + diag.blocked_by_dependency + ")");
                }
            } else {
                logger.warn("DIAGNOSIS COOLDOWN ACTIVE for " + service_name + ": suppressing repeated recovery action.");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(polling_interval_ms));
    }

    logger.info("Linux Sentinel shut down cleanly.");

    // Cleanup spawned child if active
    if (spawned_by_us && target_pid > 0 && collector->isProcessAlive(target_pid)) {
        logger.info("Stopping supervised child PID " + std::to_string(target_pid) + " via SIGTERM...");
        kill(target_pid, SIGTERM);
        int status = 0;
        waitpid(target_pid, &status, 0);
        logger.success("Child PID reaped successfully.");
    }

    return 0;
}
