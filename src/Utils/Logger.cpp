#include "../../include/Utils/Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace whot::utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : minLevel_(LogLevel::INFO)
    , consoleOutput_(true)
    , fileOutput_(false)
    , includeTimestamp_(true)
    , includeLevel_(true)
    , includeThreadId_(false)
{}

Logger::~Logger() {
    if (logFile_.is_open()) logFile_.close();
}

void Logger::setLogLevel(LogLevel level) { minLevel_ = level; }
void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) logFile_.close();
    if (!filepath.empty()) logFile_.open(filepath, std::ios::app);
}
void Logger::enableConsoleOutput(bool enable) { consoleOutput_ = enable; }
void Logger::enableFileOutput(bool enable) { fileOutput_ = enable; }
void Logger::setIncludeTimestamp(bool include) { includeTimestamp_ = include; }
void Logger::setIncludeLevel(bool include) { includeLevel_ = include; }
void Logger::setIncludeThreadId(bool include) { includeThreadId_ = include; }

void Logger::log(LogLevel level, const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(minLevel_)) return;
    std::string formatted = formatMessage(level, message);
    std::lock_guard<std::mutex> lock(mutex_);
    if (consoleOutput_) std::cout << formatted << std::endl;
    if (fileOutput_ && logFile_.is_open()) logFile_ << formatted << std::endl;
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    std::ostringstream oss;
    if (includeTimestamp_) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        oss << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "] ";
    }
    if (includeLevel_) oss << "[" << levelToString(level) << "] ";
    oss << message;
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "?";
    }
}

void Logger::debug(const std::string& message)   { log(LogLevel::DEBUG, message); }
void Logger::info(const std::string& message)   { log(LogLevel::INFO, message); }
void Logger::warning(const std::string& message) { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message)  { log(LogLevel::ERROR, message); }
void Logger::critical(const std::string& message) { log(LogLevel::CRITICAL, message); }

} // namespace whot::utils
