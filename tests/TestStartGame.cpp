#include <gtest/gtest.h>
#include "Application.hpp"
#include "TestHelpers.hpp"
#include "Network/MessageProtocol.hpp"
#include "Game/GameState.hpp"

namespace whot {

using namespace whot::test;

TEST(TestStartGame, GameStaysInLobby_UntilExplicitStart) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig cfg;
    cfg.minPlayers = 2;
    std::string gameId = app.createGame(cfg);
    app.joinGame(gameId, "creator", "Alice");
    game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getPhase(), game::GamePhase::LOBBY);
    app.joinGame(gameId, "player2", "Bob");
    eng = app.getGame(gameId);
    EXPECT_EQ(eng->getState()->getPhase(), game::GamePhase::LOBBY);
}

TEST(TestStartGame, CreatorIsFirstPlayer) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "creator-id", "Creator");
    game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getCreatorPlayerId(), "creator-id");
    app.joinGame(gameId, "other-id", "Other");
    eng = app.getGame(gameId);
    EXPECT_EQ(eng->getState()->getCreatorPlayerId(), "creator-id");
}

TEST(TestStartGame, StartGame_TransitionsToInProgress) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig cfg;
    cfg.minPlayers = 2;
    std::string gameId = app.createGame(cfg);
    app.joinGame(gameId, "creator", "Alice");
    app.joinGame(gameId, "player2", "Bob");
    std::string sid = app.getWebSocketServer()->getSessionManager()->createSession("127.0.0.1");
    app.getWebSocketServer()->getSessionManager()->setGameId(sid, gameId);
    app.getWebSocketServer()->getSessionManager()->setPlayerId(sid, "creator");
    network::Message startMsg;
    startMsg.type = network::MessageType::START_GAME;
    startMsg.gameId = gameId;
    startMsg.playerId = "creator";
    startMsg.payload = "{}";
    app.handleClientMessage(sid, startMsg);
    const game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getPhase(), game::GamePhase::IN_PROGRESS);
    EXPECT_NE(eng->getState()->getCallCard(), nullptr);
}

} // namespace whot
