#ifndef LINUX_SENTINEL_CHECKPOINT_MANAGER_H
#define LINUX_SENTINEL_CHECKPOINT_MANAGER_H

#include "Common.h"
#include <string>
#include <vector>
#include <mutex>

namespace sentinel {

class CheckpointManager {
public:
    explicit CheckpointManager(std::string snapshot_dir = "logs/snapshots");
    ~CheckpointManager() = default;

    // Capture an incident snapshot in-memory and write to disk
    IncidentSnapshot captureSnapshot(const std::string& incident_id,
                                    const std::string& service_name,
                                    int target_pid,
                                    const ProcessMetrics& metrics,
                                    const std::string& trigger_reason,
                                    const std::string& config_version = "1.0.0");

    // Retrieve an in-memory snapshot by incident ID
    std::optional<IncidentSnapshot> getSnapshot(const std::string& incident_id) const;

    // Retrieve all captured snapshots
    std::vector<IncidentSnapshot> getAllSnapshots() const;

private:
    std::string snapshot_dir_;
    mutable std::mutex mutex_;
    std::vector<IncidentSnapshot> snapshots_;

    // Write formatted snapshot to filesystem
    bool persistSnapshot(const IncidentSnapshot& snapshot);
};

} // namespace sentinel

#endif // LINUX_SENTINEL_CHECKPOINT_MANAGER_H
