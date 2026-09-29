#ifndef LINUX_SENTINEL_DIAGNOSIS_ENGINE_H
#define LINUX_SENTINEL_DIAGNOSIS_ENGINE_H

#include "Common.h"
#include "DependencyManager.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace sentinel {

class DiagnosisEngine {
public:
    explicit DiagnosisEngine(std::shared_ptr<DependencyManager> dep_mgr, 
                             uint64_t cooldown_period_ms = 5000);
    ~DiagnosisEngine() = default;

    // Diagnose root cause from anomaly report and service dependencies
    DiagnosisRecord diagnose(const AnomalyReport& anomaly);

    // Check if service is currently in a post-diagnosis cooldown window
    bool isCoolingDown(const std::string& service_name) const;

    // Reset cooldown for a service
    void resetCooldown(const std::string& service_name);

private:
    std::shared_ptr<DependencyManager> dep_mgr_;
    uint64_t cooldown_period_ms_{5000};

    // Tracks last diagnosis timestamp per service to prevent flapping
    std::unordered_map<std::string, uint64_t> last_diagnosis_timestamps_;
};

} // namespace sentinel

#endif // LINUX_SENTINEL_DIAGNOSIS_ENGINE_H
