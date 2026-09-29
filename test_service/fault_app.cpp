#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <cstring>

static volatile sig_atomic_t g_running = 1;

void handleSignal(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        std::cout << "[fault_app] Received signal " << signum 
                  << " (SIGTERM/SIGINT). Initiating graceful shutdown..." << std::endl;
        g_running = 0;
    }
}

int main(int argc, char* argv[]) {
    // Register graceful termination signal handler
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);

    std::string mode = "normal";
    std::string app_name = "fault_app";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--leak") {
            mode = "leak";
        } else if (arg == "--cpu") {
            mode = "cpu";
        } else if (arg == "--crash-on-start") {
            std::cerr << "[fault_app] Simulated immediate crash on startup!" << std::endl;
            return 1;
        } else if (arg == "--normal") {
            mode = "normal";
        } else if (arg == "--name" && i + 1 < argc) {
            app_name = argv[++i];
        }
    }

    pid_t my_pid = getpid();
    std::cout << "========================================================\n"
              << "  [TARGET APPLICATION] " << app_name << " (PID: " << my_pid << ")\n"
              << "  Operational Mode   : " << mode << "\n"
              << "========================================================" << std::endl;

    // Baseline allocation (approx 15 MB)
    std::vector<std::vector<char>> memory_holder;
    memory_holder.emplace_back(15 * 1024 * 1024, 0xAA);

    uint32_t iteration = 0;
    while (g_running) {
        iteration++;
        
        if (mode == "leak") {
            // Allocate an additional 5 MB every loop tick to simulate progressive leak
            memory_holder.emplace_back(5 * 1024 * 1024, 0xBB);
            size_t total_mb = (memory_holder.size() * 5) + 10;
            std::cout << "[fault_app PID " << my_pid << "] Leaking memory... Allocated ~" 
                      << total_mb << " MB total." << std::endl;
        } else if (mode == "cpu") {
            // Spin CPU in a tight loop for 500ms
            auto start = std::chrono::steady_clock::now();
            volatile double x = 1.0;
            while (std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start).count() < 600) {
                x = x * 1.000001 + 0.000001;
            }
            std::cout << "[fault_app PID " << my_pid << "] Generating high CPU load..." << std::endl;
        } else {
            // Normal baseline operation
            if (iteration % 5 == 1) {
                std::cout << "[fault_app PID " << my_pid << "] Operating normally (Heartbeat " 
                          << iteration << ")." << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "[fault_app PID " << my_pid << "] Clean exit completed." << std::endl;
    return 0;
}
