#ifndef WHOT_APPLICATION_HPP
#define WHOT_APPLICATION_HPP

#include "Network/WebSocketServer.hpp"
#include "Network/HTTPServer.hpp"
#include "Game/GameEngine.hpp"
#include "Persistence/Database.hpp"
#include "Persistence/GameRepository.hpp"
#include "Persistence/PlayerRepository.hpp"
#include <memory>
#include <map>
#include <string>

namespace whot {

struct ApplicationConfig {
    uint16_t websocketPort = 8080;
    uint16_t httpPort = 8081;
    std::string staticFilesPath = "./web";
    persistence::DatabaseConfig dbConfig;
    int maxGamesPerServer = 100;
    int maxPlayersPerGame = 8;
    bool enableAI = true;
    std::string logFilePath = "./logs/whot.log";
};

class Application {
public:
    explicit Application(const ApplicationConfig& config);
    ~Application();
    
    // Application lifecycle
    void initialize();
    void run();
    void shutdown();
    
    // Game management
    std::string createGame(const game::GameConfig& gameConfig);
    bool joinGame(const std::string& gameId, const std::string& playerId,
                  const std::string& playerName = "");
    void addBotsToGame(const std::string& gameId, int botCount);
    void runBotTurnsIfNeeded(const std::string& gameId);
    /// Resolve gameId from game code (empty if not found).
    std::string getGameIdByCode(const std::string& gameCode) const;
    /// Get game code for a game (empty if not found). For tests and API.
    std::string getGameCode(const std::string& gameId) const;
    bool leaveGame(const std::string& gameId, const std::string& playerId);
    void removeGame(const std::string& gameId);
    
    // Message routing
    void handleClientMessage(const std::string& sessionId,
                             const network::Message& message);
    
    // Server access
    network::WebSocketServer* getWebSocketServer();
    network::HttpServer* getHttpServer();
    game::GameEngine* getGame(const std::string& gameId);
    const game::GameEngine* getGame(const std::string& gameId) const;
    
private:
    ApplicationConfig config_;
    
    std::unique_ptr<network::WebSocketServer> wsServer_;
    std::unique_ptr<network::HttpServer> httpServer_;
    std::unique_ptr<persistence::Database> database_;
    std::unique_ptr<persistence::GameRepository> gameRepo_;
    std::unique_ptr<persistence::PlayerRepository> playerRepo_;
    
    std::map<std::string, std::unique_ptr<game::GameEngine>> activeGames_;
    mutable std::mutex gamesMutex_;
    
    // Initialization helpers
    void setupWebSocketHandlers();
    void setupHttpRoutes();
    void setupDatabase();
    void loadExistingGames();
    
    // Message handlers
    void handleJoinGame(const std::string& sessionId,
                        const network::Message& message);
    void handleStartGame(const std::string& sessionId,
                         const network::Message& message);
    void handleLeaveGame(const std::string& sessionId,
                         const network::Message& message);
    void handleGameAction(const std::string& sessionId,
                          const network::Message& message);
    
    // Utility
    void broadcastGameState(const std::string& gameId);
    void cleanupInactiveGames();

    // HTTP API handlers (used by HttpServer routes)
    network::HttpResponse handleGetGames(const network::HttpRequest& request);
    network::HttpResponse handleCreateGame(const network::HttpRequest& request);
    network::HttpResponse handleGetGame(const std::string& gameId);
    network::HttpResponse handleJoinGameHttp(const std::string& gameId,
                                             const network::HttpRequest& request);
    network::HttpResponse handleJoinByCode(const network::HttpRequest& request);
};

} // namespace whot

#endif // WHOT_APPLICATION_HPP
