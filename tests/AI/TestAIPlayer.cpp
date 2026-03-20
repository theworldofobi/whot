#include <gtest/gtest.h>
#include "AI/AIPlayer.hpp"
#include "AI/Strategy.hpp"
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"

namespace whot::ai {

using namespace whot::test;

TEST(TestAIPlayer, Construction) {
    AIPlayer ai("ai1", "Bot", DifficultyLevel::EASY);
    EXPECT_EQ(ai.getDifficulty(), DifficultyLevel::EASY);
}

TEST(TestAIPlayer, DecideAction_ReturnsAction) {
    AIPlayer ai("ai1", "Bot", DifficultyLevel::EASY);
    auto state = makeGameStateWithPlayers(1);
    game::GameAction action = ai.decideAction(*state);
    EXPECT_TRUE(action.type == game::ActionType::DRAW_CARD ||
                action.type == game::ActionType::PLAY_CARD ||
                action.type == game::ActionType::DECLARE_LAST_CARD ||
                action.type == game::ActionType::DECLARE_CHECK_UP);
}

TEST(TestAIPlayer, ChooseSuitForWhotCard) {
    AIPlayer ai("ai1", "Bot", DifficultyLevel::EASY);
    auto state = makeGameStateWithPlayers(1);
    core::Suit s = ai.chooseSuitForWhotCard(*state);
    EXPECT_GE(static_cast<int>(s), 0);
}

TEST(TestAIPlayer, SetDifficulty) {
    AIPlayer ai("ai1", "Bot", DifficultyLevel::EASY);
    ai.setDifficulty(DifficultyLevel::HARD);
    EXPECT_EQ(ai.getDifficulty(), DifficultyLevel::HARD);
}

} // namespace whot::ai
