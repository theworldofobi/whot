#include <gtest/gtest.h>
#include "Application.hpp"
#include "TestHelpers.hpp"

namespace whot {

using namespace whot::test;

TEST(TestGameCode, CreateGame_GeneratesNonEmptyGameCode) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    ASSERT_FALSE(gameId.empty());
    std::string code = app.getGameCode(gameId);
    EXPECT_FALSE(code.empty());
    EXPECT_EQ(code.size(), 6u);
}

TEST(TestGameCode, GetGameIdByCode_ReturnsGameIdForValidCode) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    ASSERT_FALSE(gameId.empty());
    std::string code = app.getGameCode(gameId);
    std::string resolved = app.getGameIdByCode(code);
    EXPECT_EQ(resolved, gameId);
}

TEST(TestGameCode, GetGameIdByCode_ReturnsEmptyForWrongCode) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string resolved = app.getGameIdByCode("WRONG1");
    EXPECT_TRUE(resolved.empty());
}

TEST(TestGameCode, JoinByCode_JoinGameWithResolvedId_Succeeds) {
    ApplicationConfig config;
    config.dbConfig = makeInMemoryDbConfig();
    config.httpPort = 0;
    config.websocketPort = 0;
    Application app(config);
    app.initialize();
    std::string gameId = app.createGame(game::GameConfig{});
    app.joinGame(gameId, "player-1", "Alice");
    std::string code = app.getGameCode(gameId);
    std::string resolvedId = app.getGameIdByCode(code);
    ASSERT_EQ(resolvedId, gameId);
    bool joined = app.joinGame(resolvedId, "player-2", "Bob");
    EXPECT_TRUE(joined);
    const game::GameEngine* eng = app.getGame(gameId);
    EXPECT_EQ(eng->getState()->getPlayerCount(), 2u);
}

} // namespace whot
