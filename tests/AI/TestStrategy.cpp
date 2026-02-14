#include <gtest/gtest.h>
#include "AI/Strategy.hpp"
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"

namespace whot::ai {

using namespace whot::test;

TEST(TestStrategy, RandomStrategy_SelectCard_EmptyPlayable_ReturnsZero) {
    RandomStrategy s;
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    size_t idx = s.selectCard(*state, {});
    EXPECT_EQ(idx, 0u);
}

TEST(TestStrategy, RandomStrategy_SelectCard_NonEmpty) {
    RandomStrategy s;
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    std::vector<size_t> playable = {0, 1};
    size_t idx = s.selectCard(*state, playable);
    EXPECT_TRUE(idx == 0u || idx == 1u);
}

TEST(TestStrategy, RandomStrategy_SelectSuit) {
    RandomStrategy s;
    auto state = makeGameStateWithPlayers(1);
    core::Suit suit = s.selectSuit(*state);
    EXPECT_GE(static_cast<int>(suit), 0);
    EXPECT_LE(static_cast<int>(suit), 4);
}

TEST(TestStrategy, RandomStrategy_ShouldDefend_EmptyDefense_ReturnsFalse) {
    RandomStrategy s;
    auto state = makeGameStateWithPlayers(1);
    core::Card attack(core::Suit::CIRCLE, core::CardValue::TWO);
    EXPECT_FALSE(s.shouldDefend(*state, attack, {}));
}

TEST(TestStrategy, RandomStrategy_GetStrategyName) {
    RandomStrategy s;
    EXPECT_EQ(s.getStrategyName(), "Random");
}

TEST(TestStrategy, AggressiveStrategy_GetStrategyName) {
    AggressiveStrategy s;
    EXPECT_FALSE(s.getStrategyName().empty());
}

TEST(TestStrategy, DefensiveStrategy_GetStrategyName) {
    DefensiveStrategy s;
    EXPECT_FALSE(s.getStrategyName().empty());
}

TEST(TestStrategy, BalancedStrategy_GetStrategyName) {
    BalancedStrategy s;
    EXPECT_FALSE(s.getStrategyName().empty());
}

TEST(TestStrategy, AggressiveStrategy_SelectCard_Empty_ReturnsZero) {
    AggressiveStrategy s;
    auto state = makeGameStateWithPlayers(1);
    EXPECT_EQ(s.selectCard(*state, {}), 0u);
}

} // namespace whot::ai
