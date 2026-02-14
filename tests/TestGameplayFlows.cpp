#include <gtest/gtest.h>
#include "Application.hpp"
#include "TestHelpers.hpp"
#include "Network/MessageProtocol.hpp"
#include "Game/GameState.hpp"

namespace whot {

using namespace whot::test;

TEST(TestGameplayFlows, Flow_CreateJoinByCodeStart_StateCorrect) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "creator", "Alice");
    std::string code = app.getGameCode(gameId);
    app.joinGame(app.getGameIdByCode(code), "player2", "Bob");
    std::string s1 = app.getWebSocketServer()->getSessionManager()->createSession("1");
    app.getWebSocketServer()->getSessionManager()->setGameId(s1, gameId);
    app.getWebSocketServer()->getSessionManager()->setPlayerId(s1, "creator");
    std::string s2 = app.getWebSocketServer()->getSessionManager()->createSession("2");
    app.getWebSocketServer()->getSessionManager()->setGameId(s2, gameId);
    app.getWebSocketServer()->getSessionManager()->setPlayerId(s2, "player2");
    network::Message startMsg;
    startMsg.type = network::MessageType::START_GAME;
    startMsg.gameId = gameId;
    startMsg.playerId = "creator";
    startMsg.payload = "{}";
    app.handleClientMessage(s1, startMsg);
    const game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getPhase(), game::GamePhase::IN_PROGRESS);
    EXPECT_EQ(eng->getState()->getCurrentPlayerIndex(), 0);
    EXPECT_EQ(eng->getState()->getPlayerCount(), 2u);
    EXPECT_NE(eng->getState()->getCallCard(), nullptr);
}

TEST(TestGameplayFlows, Visibility_OtherPlayersHandsHiddenInJson) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig cfg;
    cfg.minPlayers = 2;
    std::string gameId = app.createGame(cfg);
    app.joinGame(gameId, "alice", "Alice");
    app.joinGame(gameId, "bob", "Bob");
    std::string sid = app.getWebSocketServer()->getSessionManager()->createSession("1");
    app.getWebSocketServer()->getSessionManager()->setGameId(sid, gameId);
    app.getWebSocketServer()->getSessionManager()->setPlayerId(sid, "alice");
    network::Message startMsg;
    startMsg.type = network::MessageType::START_GAME;
    startMsg.gameId = gameId;
    startMsg.playerId = "alice";
    startMsg.payload = "{}";
    app.handleClientMessage(sid, startMsg);
    const game::GameEngine* eng = app.getGame(gameId);
    std::string jsonForBob = eng->getState()->toJsonForPlayer("bob");
    EXPECT_TRUE(jsonForBob.find("\"count\"") != std::string::npos);
}

TEST(TestGameplayFlows, TurnOrder_CreatorFirstThenClockwise) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig cfg;
    cfg.minPlayers = 2;
    std::string gameId = app.createGame(cfg);
    app.joinGame(gameId, "p0", "P0");
    app.joinGame(gameId, "p1", "P1");
    std::string sid = app.getWebSocketServer()->getSessionManager()->createSession("1");
    app.getWebSocketServer()->getSessionManager()->setGameId(sid, gameId);
    app.getWebSocketServer()->getSessionManager()->setPlayerId(sid, "p0");
    network::Message startMsg;
    startMsg.type = network::MessageType::START_GAME;
    startMsg.gameId = gameId;
    startMsg.playerId = "p0";
    startMsg.payload = "{}";
    app.handleClientMessage(sid, startMsg);
    const game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getCurrentPlayerIndex(), 0);
    const auto* cur = eng->getState()->getCurrentPlayer();
    ASSERT_NE(cur, nullptr);
    EXPECT_EQ(cur->getId(), "p0");
}

} // namespace whot
