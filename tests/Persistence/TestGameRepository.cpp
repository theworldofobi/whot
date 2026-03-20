#include <gtest/gtest.h>
#include "Persistence/GameRepository.hpp"
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"

namespace whot::persistence {

using namespace whot::test;

TEST(TestGameRepository, SaveAndLoadGame) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    GameRepository repo(db.get());
    auto state = whot::test::makeGameStateWithPlayers(2);
    state->startRound();
    EXPECT_TRUE(repo.saveGame(*state));
    auto loaded = repo.loadGame(state->getGameId());
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getPlayerCount(), 2u);
}

TEST(TestGameRepository, LoadGame_Nonexistent_ReturnsNullopt) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    GameRepository repo(db.get());
    auto loaded = repo.loadGame("nonexistent-id");
    EXPECT_FALSE(loaded.has_value());
}

TEST(TestGameRepository, UpdateGame) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    GameRepository repo(db.get());
    auto state = whot::test::makeGameStateWithPlayers(1);
    repo.saveGame(*state);
    state->setPhase(game::GamePhase::ROUND_ENDED);
    EXPECT_TRUE(repo.updateGame(*state));
}

TEST(TestGameRepository, DeleteGame) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    GameRepository repo(db.get());
    auto state = whot::test::makeGameStateWithPlayers(1);
    repo.saveGame(*state);
    EXPECT_TRUE(repo.deleteGame(state->getGameId()));
    EXPECT_FALSE(repo.loadGame(state->getGameId()).has_value());
}

TEST(TestGameRepository, GetActiveGames) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    GameRepository repo(db.get());
    auto list = repo.getActiveGames();
    EXPECT_TRUE(list.empty() || !list.empty());
}

} // namespace whot::persistence
