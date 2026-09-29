#ifndef LINUX_SENTINEL_DEPENDENCY_MANAGER_H
#define LINUX_SENTINEL_DEPENDENCY_MANAGER_H

#include "Common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace sentinel {

class DependencyManager {
public:
    DependencyManager() = default;
    ~DependencyManager() = default;

    // Register a service and its upstream dependencies (parent services)
    bool registerService(const std::string& service_name, const std::vector<std::string>& depends_on);

    // Update the live operational state of a service
    void setServiceState(const std::string& service_name, ServiceState state);

    // Get the current operational state of a service
    ServiceState getServiceState(const std::string& service_name) const;

    // Check if all upstream dependencies for a service are healthy
    // If not, returns false and populates failing_dependency with the first non-healthy parent
    bool areDependenciesHealthy(const std::string& service_name, std::string& failing_dependency) const;

    // Retrieve direct dependencies
    std::vector<std::string> getDependencies(const std::string& service_name) const;

    // Reset all service states
    void clear();

private:
    // DAG Adjacency List: service -> list of upstream services it depends on
    std::unordered_map<std::string, std::vector<std::string>> dependency_graph_;

    // Live operational state per service
    std::unordered_map<std::string, ServiceState> service_states_;

    // Cycle detection helper during registration
    bool hasCycleDFS(const std::string& current, 
                     std::unordered_set<std::string>& visited, 
                     std::unordered_set<std::string>& recursion_stack) const;
};

} // namespace sentinel

#endif // LINUX_SENTINEL_DEPENDENCY_MANAGER_H
