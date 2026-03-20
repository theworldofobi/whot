#include "../include/Application.hpp"
#include "../include/Utils/Logger.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <cstdlib>
#include <atomic>

// Global application pointer for signal handling
std::unique_ptr<whot::Application> g_app;

// Set by signal handler to request shutdown (defined in Application.cpp).
extern std::atomic<bool> g_shutdown;

void signalHandler(int signal) {
    (void)signal;
    g_shutdown.store(true, std::memory_order_relaxed);
}

int main(int argc, char** argv) {
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Configure application
    whot::ApplicationConfig config;
    std::string dbPath = "./whot.db";
    if (const char* envDbPath = std::getenv("WHOT_DB_PATH")) {
        std::string val(envDbPath);
        if (!val.empty()) dbPath = val;
    }
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--ws-port" && i + 1 < argc) {
            config.websocketPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--http-port" && i + 1 < argc) {
            config.httpPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--static-path" && i + 1 < argc) {
            config.staticFilesPath = argv[++i];
        } else if (arg == "--log-file" && i + 1 < argc) {
            config.logFilePath = argv[++i];
        } else if (arg == "--db-path" && i + 1 < argc) {
            dbPath = argv[++i];
        } else if (arg == "--no-ai") {
            config.enableAI = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Whot Card Game Server\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --ws-port PORT       WebSocket port (default: 8080)\n";
            std::cout << "  --http-port PORT     HTTP port (default: 8081)\n";
            std::cout << "  --static-path PATH   Static files path (default: ./web)\n";
            std::cout << "  --log-file PATH      Log file path (default: ./logs/whot.log)\n";
            std::cout << "  --db-path PATH       SQLite DB file path (default: ./whot.db or $WHOT_DB_PATH)\n";
            std::cout << "  --no-ai              Disable AI players\n";
            std::cout << "  --help, -h           Show this help message\n";
            return 0;
        }
    }
    
    // Configure database
    config.dbConfig.type = whot::persistence::DatabaseType::SQLITE;
    config.dbConfig.filepath = dbPath;
    
    try {
        // Initialize logger
        auto& logger = whot::utils::Logger::getInstance();
        logger.setLogFile(config.logFilePath);
        logger.setLogLevel(whot::utils::LogLevel::INFO);
        logger.enableConsoleOutput(true);
        logger.enableFileOutput(true);
        
        LOG_INFO("Starting Whot Card Game Server");
        LOG_INFO("WebSocket Port: " + std::to_string(config.websocketPort));
        LOG_INFO("HTTP Port: " + std::to_string(config.httpPort));
        
        // Create and initialize application
        g_app = std::make_unique<whot::Application>(config);
        g_app->initialize();
        
        LOG_INFO("Server initialized successfully");
        std::cout << "\n";
        std::cout << "=================================\n";
        std::cout << "  Whot Card Game Server Ready\n";
        std::cout << "=================================\n";
        std::cout << "WebSocket: ws://localhost:" << config.websocketPort << " (use this port for WS, not HTTP)\n";
        std::cout << "HTTP:      http://localhost:" << config.httpPort << "\n";
        std::cout << "Static:    " << config.staticFilesPath << "\n";
        std::cout << "\nPress Ctrl+C once to stop the server\n";
        std::cout << "=================================\n\n";
        
        // Run the application (blocks until shutdown)
        g_app->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_CRITICAL("Fatal error: " + std::string(e.what()));
        return 1;
    }
    
    LOG_INFO("Server shut down successfully");
    return 0;
}
