#include "Logger.h"
#include <iomanip>
#include <filesystem>

namespace sentinel {

namespace colors {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[1;31m";
    const std::string GREEN   = "\033[1;32m";
    const std::string YELLOW  = "\033[1;33m";
    const std::string BLUE    = "\033[1;34m";
    const std::string CYAN    = "\033[1;36m";
    const std::string BOLD    = "\033[1m";
}

void Logger::init(const std::string& log_file_path, bool enable_color) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_path_ = log_file_path;
    enable_color_ = enable_color;

    // Ensure parent directory exists
    std::filesystem::path p(log_file_path_);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    file_stream_.open(log_file_path_, std::ios::out | std::ios::app);
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::log(const std::string& level, const std::string& msg, const std::string& color_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now_ms = getCurrentTimeMs();
    std::string time_str = formatTimestamp(now_ms);

    // Terminal Output
    if (enable_color_) {
        std::cout << colors::CYAN << "[" << time_str << "] " << colors::RESET
                  << color_code << "[" << level << "] " << colors::RESET
                  << msg << std::endl;
    } else {
        std::cout << "[" << time_str << "] [" << level << "] " << msg << std::endl;
    }

    // File Output (Raw text, no ANSI colors)
    if (file_stream_.is_open()) {
        file_stream_ << "[" << time_str << "] [" << level << "] " << msg << "\n";
        file_stream_.flush();
    }
}

void Logger::info(const std::string& msg) {
    log("INFO", msg, colors::BLUE);
}

void Logger::warn(const std::string& msg) {
    log("WARN", msg, colors::YELLOW);
}

void Logger::error(const std::string& msg) {
    log("ERROR", msg, colors::RED);
}

void Logger::success(const std::string& msg) {
    log("SUCCESS", msg, colors::GREEN);
}

void Logger::logIncident(const IncidentCard& card) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string card_text = card.to_formatted_card();

    // Print to terminal in bold yellow/cyan
    if (enable_color_) {
        std::cout << "\n" << colors::YELLOW << card_text << colors::RESET << std::endl;
    } else {
        std::cout << "\n" << card_text << std::endl;
    }

    // Write to incident log file
    if (file_stream_.is_open()) {
        file_stream_ << "\n" << card_text << "\n";
        file_stream_.flush();
    }
}

std::string Logger::generateIncidentId() {
    uint32_t id = incident_counter_++;
    std::ostringstream oss;
    oss << "INC-" << std::setfill('0') << std::setw(4) << id;
    return oss.str();
}

} // namespace sentinel
