#include <gtest/gtest.h>
#include "Application.hpp"
#include "TestHelpers.hpp"
#include "Core/Player.hpp"
#include "Game/GameState.hpp"

namespace whot {

using namespace whot::test;

TEST(TestBots, CreateGame_WithBotCount_AddsBots) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "human", "Human");
    app.addBotsToGame(gameId, 2);
    game::GameEngine* eng = app.getGame(gameId);
    ASSERT_NE(eng, nullptr);
    EXPECT_EQ(eng->getState()->getPlayerCount(), 3u);
    const auto* p0 = eng->getState()->getPlayer("human");
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(p0->getType(), core::PlayerType::HUMAN);
    bool hasBot1 = eng->getState()->getPlayer("bot-" + gameId + "-0") != nullptr;
    bool hasBot2 = eng->getState()->getPlayer("bot-" + gameId + "-1") != nullptr;
    EXPECT_TRUE(hasBot1);
    EXPECT_TRUE(hasBot2);
}

TEST(TestBots, BotsHaveAI_Type) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "human", "H");
    app.addBotsToGame(gameId, 1);
    game::GameEngine* eng = app.getGame(gameId);
    core::Player* bot = eng->getState()->getPlayer("bot-" + gameId + "-0");
    ASSERT_NE(bot, nullptr);
    EXPECT_EQ(bot->getType(), core::PlayerType::AI_EASY);
}

TEST(TestBots, NoAutoStart_WithBots) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    game::GameConfig cfg;
    cfg.minPlayers = 2;
    std::string gameId = app.createGame(cfg);
    app.joinGame(gameId, "human", "H");
    app.addBotsToGame(gameId, 1);
    game::GameEngine* eng = app.getGame(gameId);
    EXPECT_EQ(eng->getState()->getPhase(), game::GamePhase::LOBBY);
}

} // namespace whot
