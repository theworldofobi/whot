#include <gtest/gtest.h>
#include "Application.hpp"
#include "TestHelpers.hpp"
#include "Network/MessageProtocol.hpp"

namespace whot {

using namespace whot::test;

TEST(TestIntegration, CreateGame_ReturnsNonEmptyId) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig gameConfig;
    std::string gameId = app.createGame(gameConfig);
    EXPECT_FALSE(gameId.empty());
}

TEST(TestIntegration, JoinGame_ValidGame) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig gameConfig;
    std::string gameId = app.createGame(gameConfig);
    ASSERT_FALSE(gameId.empty());
    bool joined = app.joinGame(gameId, "player-1", "Alice");
    EXPECT_TRUE(joined);
}

TEST(TestIntegration, JoinGame_NonexistentGame_ReturnsFalse) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    Application app(config);
    app.initialize();
    EXPECT_FALSE(app.joinGame("nonexistent-game-id", "p1", "P"));
}

TEST(TestIntegration, HandleClientMessage_JoinGame_WithValidSession) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    ASSERT_FALSE(gameId.empty());
    std::string sessionId = app.getWebSocketServer()->getSessionManager()->createSession("127.0.0.1");
    network::Message msg = makeMessage(network::MessageType::JOIN_GAME,
        "{\"gameId\":\"" + gameId + "\",\"playerName\":\"P1\"}");
    app.handleClientMessage(sessionId, msg);
    auto sessions = app.getWebSocketServer()->getSessionManager()->getSessionsForGame(gameId);
    EXPECT_EQ(sessions.size(), 1u);
}

TEST(TestIntegration, HandleClientMessage_InvalidGameId_NoCrash) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string sessionId = app.getWebSocketServer()->getSessionManager()->createSession("1");
    network::Message msg = makeMessage(network::MessageType::JOIN_GAME, "{\"gameId\":\"\",\"playerName\":\"x\"}");
    app.handleClientMessage(sessionId, msg);
}

TEST(TestIntegration, HandleClientMessage_MalformedPayload_NoCrash) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string sessionId = app.getWebSocketServer()->getSessionManager()->createSession("1");
    network::Message msg = makeMessage(network::MessageType::JOIN_GAME, "not json");
    app.handleClientMessage(sessionId, msg);
}

TEST(TestIntegration, LeaveGame_RemoveGame) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "p1", "P1");
    EXPECT_TRUE(app.leaveGame(gameId, "p1"));
    app.removeGame(gameId);
}

} // namespace whot
