#include <gtest/gtest.h>
#include "Game/GameEngine.hpp"
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"

namespace whot::game {

using namespace whot::test;

TEST(TestGameEngine, StartGameStartNewRound) {
    auto state = makeGameStateWithPlayers(2);
    GameEngine engine(std::move(state));
    engine.startGame();
    engine.startNewRound();
    ASSERT_NE(engine.getState(), nullptr);
    EXPECT_EQ(engine.getState()->getPhase(), GamePhase::IN_PROGRESS);
}

TEST(TestGameEngine, ProcessAction_InvalidPlayerId) {
    auto state = makeGameStateWithPlayers(2);
    GameEngine engine(std::move(state));
    engine.startNewRound();
    GameAction action;
    action.type = ActionType::PLAY_CARD;
    action.playerId = "wrong-player";
    action.cardIndex = 0;
    ActionResult r = engine.processAction(action);
    EXPECT_FALSE(r.success);
}

TEST(TestGameEngine, ProcessAction_InvalidCardIndex) {
    auto state = makeGameStateWithPlayers(1);
    GameEngine engine(std::move(state));
    engine.startNewRound();
    GameAction action;
    action.type = ActionType::PLAY_CARD;
    action.playerId = engine.getState()->getCurrentPlayer()->getId();
    action.cardIndex = 99;
    ActionResult r = engine.processAction(action);
    EXPECT_FALSE(r.success);
}

TEST(TestGameEngine, ProcessAction_UnhandledType_ReturnsFailure) {
    auto state = makeGameStateWithPlayers(1);
    GameEngine engine(std::move(state));
    engine.startNewRound();
    GameAction action;
    action.type = ActionType::FORFEIT_TURN;
    action.playerId = engine.getState()->getCurrentPlayer()->getId();
    ActionResult r = engine.processAction(action);
    EXPECT_FALSE(r.success);
}

TEST(TestGameEngine, IsValidAction_WrongPlayer_ReturnsFalse) {
    auto state = makeGameStateWithPlayers(2);
    GameEngine engine(std::move(state));
    engine.startNewRound();
    GameAction bad;
    bad.type = ActionType::DRAW_CARD;
    bad.playerId = "wrong-player-id";
    EXPECT_FALSE(engine.isValidAction(bad));
}

TEST(TestGameEngine, GetValidActionsForCurrentPlayer_NonEmpty) {
    auto state = makeGameStateWithPlayers(2);
    GameEngine engine(std::move(state));
    engine.startNewRound();
    auto actions = engine.getValidActionsForCurrentPlayer();
    EXPECT_FALSE(actions.empty());
}

TEST(TestGameEngine, EventCallback) {
    auto state = makeGameStateWithPlayers(1);
    GameEngine engine(std::move(state));
    bool fired = false;
    engine.registerEventCallback("round_started", [&fired](const std::string&, const std::string&) { fired = true; });
    engine.startNewRound();
    EXPECT_TRUE(fired);
}

} // namespace whot::game
