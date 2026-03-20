#include <gtest/gtest.h>
#include "Game/GameState.hpp"
#include "Game/TurnManager.hpp"
#include "TestHelpers.hpp"

namespace whot::game {

using namespace whot::test;

TEST(TestTurnManager, StartTurnEndTurn) {
    auto state = makeGameStateWithPlayers(2);
    state->startRound();
    TurnManager tm(state.get());
    tm.startTurn();
    EXPECT_FALSE(tm.getCurrentPlayerId().empty());
    tm.endTurn();
}

TEST(TestTurnManager, GetCurrentPlayerId_NoPlayers) {
    GameConfig cfg;
    GameState state(cfg);
    state.initialize();
    TurnManager tm(&state);
    EXPECT_TRUE(tm.getCurrentPlayerId().empty());
}

TEST(TestTurnManager, IsPlayerTurn) {
    auto state = makeGameStateWithPlayers(2);
    state->startRound();
    TurnManager tm(state.get());
    std::string current = tm.getCurrentPlayerId();
    EXPECT_TRUE(tm.isPlayerTurn(current));
    EXPECT_FALSE(tm.isPlayerTurn("wrong-id"));
}

TEST(TestTurnManager, RecordActionGetTurnHistory) {
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    TurnManager tm(state.get());
    TurnAction a;
    a.playerId = "p1";
    a.action = ActionType::DRAW_CARD;
    a.completed = true;
    tm.recordAction(a);
    auto history = tm.getTurnHistory(10);
    EXPECT_GE(history.size(), 1u);
}

TEST(TestTurnManager, QueueSkipIsPlayerSkipped) {
    auto state = makeGameStateWithPlayers(2);
    TurnManager tm(state.get());
    tm.queueSkip("player-0");
    EXPECT_TRUE(tm.isPlayerSkipped("player-0"));
    EXPECT_FALSE(tm.isPlayerSkipped("player-1"));
}

TEST(TestTurnManager, CanPlayAgain) {
    auto state = makeGameStateWithPlayers(1);
    TurnManager tm(state.get());
    EXPECT_FALSE(tm.canPlayAgain());
    tm.enableMultipleActions();
    EXPECT_TRUE(tm.canPlayAgain());
    tm.disableMultipleActions();
    EXPECT_FALSE(tm.canPlayAgain());
}

TEST(TestTurnManager, GetRemainingTime_HasTurnExpired) {
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    TurnManager tm(state.get());
    auto rem = tm.getRemainingTime();
    EXPECT_GE(rem.count(), 0);
}

} // namespace whot::game
