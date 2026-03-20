#include <gtest/gtest.h>
#include "Persistence/PlayerRepository.hpp"
#include "Core/Player.hpp"
#include "TestHelpers.hpp"

namespace whot::persistence {

using namespace whot::test;

TEST(TestPlayerRepository, SaveAndLoadPlayer) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    PlayerRepository repo(db.get());
    core::Player p("pid1", "Alice", core::PlayerType::HUMAN);
    EXPECT_TRUE(repo.savePlayer(p));
    auto loaded = repo.loadPlayer("pid1");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getId(), "pid1");
    EXPECT_EQ(loaded->getName(), "Alice");
}

TEST(TestPlayerRepository, LoadPlayer_Nonexistent_ReturnsNullopt) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    PlayerRepository repo(db.get());
    auto loaded = repo.loadPlayer("nonexistent");
    EXPECT_FALSE(loaded.has_value());
}

TEST(TestPlayerRepository, UpdateAndDeletePlayer) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    PlayerRepository repo(db.get());
    core::Player p("pid2", "Bob", core::PlayerType::HUMAN);
    repo.savePlayer(p);
    p.setName("Robert");
    EXPECT_TRUE(repo.updatePlayer(p));
    EXPECT_TRUE(repo.deletePlayer("pid2"));
    EXPECT_FALSE(repo.loadPlayer("pid2").has_value());
}

} // namespace whot::persistence
