#ifndef LINUX_SENTINEL_VERIFICATION_ENGINE_H
#define LINUX_SENTINEL_VERIFICATION_ENGINE_H

#include "Common.h"
#include "HealthCollector.h"
#include <string>
#include <memory>

namespace sentinel {

class VerificationEngine {
public:
    explicit VerificationEngine(std::shared_ptr<HealthCollector> collector);
    ~VerificationEngine() = default;

    // Verify that a restarted process stabilizes and remains healthy
    VerificationResult verifyProcess(int new_pid, 
                                     const std::string& service_name, 
                                     long expected_max_rss_kb = 102400, 
                                     uint32_t stabilization_ms = 2000);

private:
    std::shared_ptr<HealthCollector> collector_;
};

} // namespace sentinel

#endif // LINUX_SENTINEL_VERIFICATION_ENGINE_H
