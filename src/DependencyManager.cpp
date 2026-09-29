#include "DependencyManager.h"
#include <iostream>

namespace sentinel {

bool DependencyManager::hasCycleDFS(const std::string& current, 
                                    std::unordered_set<std::string>& visited, 
                                    std::unordered_set<std::string>& recursion_stack) const {
    visited.insert(current);
    recursion_stack.insert(current);

    auto it = dependency_graph_.find(current);
    if (it != dependency_graph_.end()) {
        for (const auto& parent : it->second) {
            if (visited.find(parent) == visited.end()) {
                if (hasCycleDFS(parent, visited, recursion_stack)) {
                    return true;
                }
            } else if (recursion_stack.find(parent) != recursion_stack.end()) {
                return true; // Cycle detected
            }
        }
    }

    recursion_stack.erase(current);
    return false;
}

bool DependencyManager::registerService(const std::string& service_name, 
                                        const std::vector<std::string>& depends_on) {
    dependency_graph_[service_name] = depends_on;
    service_states_[service_name] = ServiceState::HEALTHY;

    // Verify acyclic property (DAG validation)
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    for (const auto& pair : dependency_graph_) {
        if (visited.find(pair.first) == visited.end()) {
            if (hasCycleDFS(pair.first, visited, recursion_stack)) {
                // Rollback registration
                dependency_graph_.erase(service_name);
                service_states_.erase(service_name);
                return false; // Registration failed: Circular dependency detected
            }
        }
    }
    return true;
}

void DependencyManager::setServiceState(const std::string& service_name, ServiceState state) {
    service_states_[service_name] = state;
}

ServiceState DependencyManager::getServiceState(const std::string& service_name) const {
    auto it = service_states_.find(service_name);
    if (it != service_states_.end()) {
        return it->second;
    }
    return ServiceState::STOPPED;
}

bool DependencyManager::areDependenciesHealthy(const std::string& service_name, 
                                               std::string& failing_dependency) const {
    auto it = dependency_graph_.find(service_name);
    if (it == dependency_graph_.end()) {
        return true; // No dependencies registered
    }

    for (const auto& parent : it->second) {
        if (parent == "none" || parent.empty()) continue;

        auto state_it = service_states_.find(parent);
        if (state_it == service_states_.end()) {
            failing_dependency = parent + " (UNREGISTERED)";
            return false;
        }

        ServiceState parent_state = state_it->second;
        if (parent_state == ServiceState::CRITICAL || 
            parent_state == ServiceState::STOPPED || 
            parent_state == ServiceState::SAFE_MODE) {
            failing_dependency = parent + " (" + serviceStateToString(parent_state) + ")";
            return false;
        }
    }

    return true;
}

std::vector<std::string> DependencyManager::getDependencies(const std::string& service_name) const {
    auto it = dependency_graph_.find(service_name);
    if (it != dependency_graph_.end()) {
        return it->second;
    }
    return {};
}

void DependencyManager::clear() {
    dependency_graph_.clear();
    service_states_.clear();
}

} // namespace sentinel
