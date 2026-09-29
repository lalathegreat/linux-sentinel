#include "CheckpointManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace sentinel {

CheckpointManager::CheckpointManager(std::string snapshot_dir)
    : snapshot_dir_(std::move(snapshot_dir)) {
    try {
        std::filesystem::create_directories(snapshot_dir_);
    } catch (...) {}
}

IncidentSnapshot CheckpointManager::captureSnapshot(const std::string& incident_id,
                                                   const std::string& service_name,
                                                   int target_pid,
                                                   const ProcessMetrics& metrics,
                                                   const std::string& trigger_reason,
                                                   const std::string& config_version) {
    std::lock_guard<std::mutex> lock(mutex_);

    IncidentSnapshot snapshot;
    snapshot.incident_id = incident_id;
    snapshot.timestamp_ms = getCurrentTimeMs();
    snapshot.service_name = service_name;
    snapshot.target_pid = target_pid;
    snapshot.metrics = metrics;
    snapshot.trigger_reason = trigger_reason;
    snapshot.config_version = config_version;

    snapshots_.push_back(snapshot);
    persistSnapshot(snapshot);

    return snapshot;
}

bool CheckpointManager::persistSnapshot(const IncidentSnapshot& snapshot) {
    std::string filename = snapshot_dir_ + "/" + snapshot.incident_id + ".snapshot";
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    out << "{\n"
        << "  \"incident_id\": \"" << snapshot.incident_id << "\",\n"
        << "  \"timestamp\": \"" << formatTimestamp(snapshot.timestamp_ms) << "\",\n"
        << "  \"service_name\": \"" << snapshot.service_name << "\",\n"
        << "  \"target_pid\": " << snapshot.target_pid << ",\n"
        << "  \"config_version\": \"" << snapshot.config_version << "\",\n"
        << "  \"trigger_reason\": \"" << snapshot.trigger_reason << "\",\n"
        << "  \"metrics_before_action\": {\n"
        << "    \"cpu_percent\": " << snapshot.metrics.cpu_percent << ",\n"
        << "    \"rss_kb\": " << snapshot.metrics.rss_kb << ",\n"
        << "    \"vmsize_kb\": " << snapshot.metrics.vmsize_kb << ",\n"
        << "    \"threads\": " << snapshot.metrics.num_threads << ",\n"
        << "    \"state\": \"" << snapshot.metrics.state_char << "\"\n"
        << "  }\n"
        << "}\n";

    return true;
}

std::optional<IncidentSnapshot> CheckpointManager::getSnapshot(const std::string& incident_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& s : snapshots_) {
        if (s.incident_id == incident_id) return s;
    }
    return std::nullopt;
}

std::vector<IncidentSnapshot> CheckpointManager::getAllSnapshots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_;
}

} // namespace sentinel
