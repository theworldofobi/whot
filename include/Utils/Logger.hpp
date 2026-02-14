#ifndef WHOT_UTILS_LOGGER_HPP
#define WHOT_UTILS_LOGGER_HPP

#include <string>
#include <fstream>
#include <memory>
#include <mutex>

namespace whot::utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();
    
    // Logging methods
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // Configuration
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filepath);
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);
    
    // Formatting
    void setIncludeTimestamp(bool include);
    void setIncludeLevel(bool include);
    void setIncludeThreadId(bool include);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void log(LogLevel level, const std::string& message);
    std::string formatMessage(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
    
    LogLevel minLevel_;
    std::ofstream logFile_;
    std::mutex mutex_;
    
    bool consoleOutput_;
    bool fileOutput_;
    bool includeTimestamp_;
    bool includeLevel_;
    bool includeThreadId_;
};

// Convenience macros
#define LOG_DEBUG(msg) whot::utils::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) whot::utils::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) whot::utils::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) whot::utils::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) whot::utils::Logger::getInstance().critical(msg)

} // namespace whot::utils

#endif // WHOT_UTILS_LOGGER_HPP
