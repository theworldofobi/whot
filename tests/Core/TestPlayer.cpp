#include <gtest/gtest.h>
#include "Core/Player.hpp"
#include "Core/Card.hpp"
#include "TestHelpers.hpp"
#include <nlohmann/json.hpp>

namespace whot::core {

using namespace whot::test;

TEST(TestPlayer, ConstructEmptyIdAndName) {
    Player p("", "", PlayerType::HUMAN);
    EXPECT_TRUE(p.getId().empty());
    EXPECT_TRUE(p.getName().empty());
    EXPECT_EQ(p.getType(), PlayerType::HUMAN);
}

TEST(TestPlayer, GetHandScoresStatus) {
    Player p("id1", "Alice", PlayerType::HUMAN);
    EXPECT_EQ(p.getId(), "id1");
    EXPECT_EQ(p.getName(), "Alice");
    EXPECT_EQ(p.getCurrentScore(), 0);
    EXPECT_EQ(p.getCumulativeScore(), 0);
    EXPECT_EQ(p.getStatus(), PlayerStatus::ACTIVE);
    p.getHand().addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    EXPECT_EQ(p.getHand().size(), 1u);
}

TEST(TestPlayer, SetName) {
    Player p("id", "Old", PlayerType::HUMAN);
    p.setName("New");
    EXPECT_EQ(p.getName(), "New");
}

TEST(TestPlayer, AddToScoreResetScores) {
    Player p("id", "P", PlayerType::HUMAN);
    p.addToScore(10);
    EXPECT_EQ(p.getCurrentScore(), 10);
    EXPECT_EQ(p.getCumulativeScore(), 10);
    p.addToScore(5);
    EXPECT_EQ(p.getCurrentScore(), 15);
    p.resetCurrentScore();
    EXPECT_EQ(p.getCurrentScore(), 0);
    EXPECT_EQ(p.getCumulativeScore(), 15);
    p.resetAllScores();
    EXPECT_EQ(p.getCumulativeScore(), 0);
}

TEST(TestPlayer, LastCardCheckUpFlags) {
    Player p("id", "P", PlayerType::HUMAN);
    EXPECT_FALSE(p.hasSaidLastCard());
    EXPECT_FALSE(p.hasSaidCheckUp());
    p.setSaidLastCard(true);
    p.setSaidCheckUp(true);
    EXPECT_TRUE(p.hasSaidLastCard());
    EXPECT_TRUE(p.hasSaidCheckUp());
    p.resetTurnFlags();
    EXPECT_FALSE(p.hasSaidLastCard());
    EXPECT_FALSE(p.hasSaidCheckUp());
}

TEST(TestPlayer, HasExceededTurnTime_ZeroMax_ReturnsFalse) {
    Player p("id", "P", PlayerType::HUMAN);
    EXPECT_FALSE(p.hasExceededTurnTime(0));
}

TEST(TestPlayer, GamesPlayedWon) {
    Player p("id", "P", PlayerType::HUMAN);
    EXPECT_EQ(p.getGamesPlayed(), 0);
    EXPECT_EQ(p.getGamesWon(), 0);
    p.incrementGamesPlayed();
    p.incrementGamesWon();
    EXPECT_EQ(p.getGamesPlayed(), 1);
    EXPECT_EQ(p.getGamesWon(), 1);
}

TEST(TestPlayer, SetStatus) {
    Player p("id", "P", PlayerType::HUMAN);
    p.setStatus(PlayerStatus::ELIMINATED);
    EXPECT_EQ(p.getStatus(), PlayerStatus::ELIMINATED);
}

TEST(TestPlayer, ToJsonFromJson_Minimal) {
    Player p("id1", "Name", PlayerType::HUMAN);
    std::string json = p.toJson();
    auto restored = Player::fromJson(json);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getId(), "id1");
    EXPECT_EQ(restored->getName(), "Name");
}

TEST(TestPlayer, FromJson_InvalidJson_Throws) {
    EXPECT_THROW(Player::fromJson(""), nlohmann::json::parse_error);
    EXPECT_THROW(Player::fromJson("not json"), nlohmann::json::parse_error);
}

TEST(TestPlayer, FromJson_EmptyObject_Defaults) {
    auto p = Player::fromJson("{}");
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(p->getId().empty());
    EXPECT_TRUE(p->getName().empty());
}

} // namespace whot::core
