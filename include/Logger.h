#ifndef LINUX_SENTINEL_LOGGER_H
#define LINUX_SENTINEL_LOGGER_H

#include "Common.h"
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <atomic>

namespace sentinel {

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void init(const std::string& log_file_path, bool enable_color = true);
    
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void success(const std::string& msg);
    
    void logIncident(const IncidentCard& card);
    std::string generateIncidentId();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const std::string& level, const std::string& msg, const std::string& color_code);

    std::string log_file_path_{"logs/incidents.log"};
    std::ofstream file_stream_;
    std::mutex mutex_;
    bool enable_color_{true};
    std::atomic<uint32_t> incident_counter_{1};
};

} // namespace sentinel

#endif // LINUX_SENTINEL_LOGGER_H
