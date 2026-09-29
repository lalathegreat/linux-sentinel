#include "ConfigManager.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>

void testValidConfig() {
    sentinel::ConfigManager mgr;
    bool ok = mgr.loadFromFile("config/sentinel.conf");
    if (!ok) {
        std::cerr << "Config load failed: " << mgr.getLastError() << std::endl;
    }
    assert(ok);
    assert(mgr.getGlobalConfig().polling_interval_ms == 1000);
    assert(mgr.getGlobalConfig().log_file_path == "logs/incidents.log");
    assert(mgr.getDetectionConfig().consecutive_samples_required == 3);
    assert(mgr.getRecoveryConfig().circuit_breaker_max_retries == 3);
    assert(mgr.getServices().find("fault_app") != mgr.getServices().end());
    std::cout << "[PASS] Valid configuration parsed and validated." << std::endl;
}

void testMissingConfig() {
    sentinel::ConfigManager mgr;
    bool ok = mgr.loadFromFile("nonexistent_path/no_file.conf");
    assert(!ok);
    assert(!mgr.getLastError().empty());
    std::cout << "[PASS] Missing configuration cleanly rejected." << std::endl;
}

void testMalformedConfig() {
    std::string bad_file = "logs/bad_test.conf";
    {
        std::ofstream ofs(bad_file);
        ofs << "[global]\npolling_interval_ms = 10\n"; // Below 50ms min limit
    }
    sentinel::ConfigManager mgr;
    bool ok = mgr.loadFromFile(bad_file);
    assert(!ok);
    assert(!mgr.getLastError().empty());
    std::filesystem::remove(bad_file);
    std::cout << "[PASS] Out-of-range configuration cleanly rejected." << std::endl;
}

int main() {
    std::cout << "Running ConfigManager Unit Tests..." << std::endl;
    testValidConfig();
    testMissingConfig();
    testMalformedConfig();
    std::cout << "All ConfigManager Tests Passed Successfully!" << std::endl;
    return 0;
}
