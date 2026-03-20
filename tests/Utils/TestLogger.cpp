#include <gtest/gtest.h>
#include "Utils/Logger.hpp"

namespace whot::utils {

TEST(TestLogger, GetInstance) {
    Logger& L = Logger::getInstance();
    (void)L;
}

TEST(TestLogger, LogLevels_NoCrash) {
    Logger& L = Logger::getInstance();
    L.setLogLevel(LogLevel::DEBUG);
    L.debug("");
    L.debug("msg");
    L.info("");
    L.info("info");
    L.warning("warn");
    L.error("err");
    L.critical("crit");
}

TEST(TestLogger, SetLogLevel) {
    Logger& L = Logger::getInstance();
    L.setLogLevel(LogLevel::INFO);
    L.setLogLevel(LogLevel::ERROR);
}

TEST(TestLogger, EnableConsoleAndFile) {
    Logger& L = Logger::getInstance();
    L.enableConsoleOutput(false);
    L.enableConsoleOutput(true);
    L.enableFileOutput(false);
    L.enableFileOutput(true);
}

TEST(TestLogger, FormatOptions) {
    Logger& L = Logger::getInstance();
    L.setIncludeTimestamp(true);
    L.setIncludeLevel(true);
    L.setIncludeThreadId(false);
    L.info("formatted");
}

} // namespace whot::utils
