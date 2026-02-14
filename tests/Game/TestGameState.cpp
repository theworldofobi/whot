#include <gtest/gtest.h>
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"
#include <nlohmann/json.hpp>

namespace whot::game {

using namespace whot::test;

TEST(TestGameState, DefaultConfig) {
    GameConfig cfg;
    GameState state(cfg);
    state.initialize();
    EXPECT_EQ(state.getPhase(), GamePhase::LOBBY);
    EXPECT_EQ(state.getConfig().minPlayers, 2);
    EXPECT_EQ(state.getConfig().maxPlayers, 8);
    EXPECT_FALSE(state.getGameId().empty());
}

TEST(TestGameState, AddPlayerRemovePlayer) {
    auto state = makeGameStateWithPlayers(2);
    EXPECT_EQ(state->getPlayerCount(), 2u);
    state->removePlayer("player-0");
    EXPECT_EQ(state->getPlayerCount(), 1u);
    state->removePlayer("nonexistent");
    EXPECT_EQ(state->getPlayerCount(), 1u);
}

TEST(TestGameState, GetPlayer_NullId) {
    auto state = makeGameStateWithPlayers(1);
    EXPECT_EQ(state->getPlayer(""), nullptr);
    EXPECT_EQ(state->getPlayer("nonexistent"), nullptr);
}

TEST(TestGameState, GetCurrentPlayer_ZeroPlayers) {
    GameConfig cfg;
    GameState state(cfg);
    state.initialize();
    EXPECT_EQ(state.getCurrentPlayer(), nullptr);
}

TEST(TestGameState, GetCurrentPlayer_OnePlayer) {
    auto state = makeGameStateWithPlayers(1);
    ASSERT_NE(state->getCurrentPlayer(), nullptr);
    EXPECT_EQ(state->getCurrentPlayer()->getId(), "player-0");
}

TEST(TestGameState, AdvanceTurnReverseDirection) {
    auto state = makeGameStateWithPlayers(2);
    EXPECT_EQ(state->getCurrentPlayerIndex(), 0);
    state->advanceTurn();
    EXPECT_EQ(state->getCurrentPlayerIndex(), 1);
    state->advanceTurn();
    EXPECT_EQ(state->getCurrentPlayerIndex(), 0);
    state->reverseDirection();
    state->advanceTurn();
    EXPECT_EQ(state->getCurrentPlayerIndex(), 1);
}

TEST(TestGameState, SetCallCardAddToDiscardPile) {
    auto state = makeGameStateWithPlayers(1);
    state->setCallCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::FIVE));
    ASSERT_NE(state->getCallCard(), nullptr);
    EXPECT_EQ(state->getCallCard()->getSuit(), core::Suit::CIRCLE);
    state->addToDiscardPile(std::make_unique<core::Card>(core::Suit::TRIANGLE, core::CardValue::ONE));
}

TEST(TestGameState, ActivePickCountDemandedSuit) {
    auto state = makeGameStateWithPlayers(2);
    state->setActivePickCount(4);
    EXPECT_EQ(state->getActivePickCount(), 4);
    state->resetActivePickCount();
    EXPECT_EQ(state->getActivePickCount(), 0);
    state->setDemandedSuit(core::Suit::BLOCK);
    ASSERT_TRUE(state->getDemandedSuit().has_value());
    EXPECT_EQ(*state->getDemandedSuit(), core::Suit::BLOCK);
    state->clearDemandedSuit();
    EXPECT_FALSE(state->getDemandedSuit().has_value());
}

TEST(TestGameState, ToJsonForPlayer_MasksOtherHands) {
    auto state = makeGameStateWithPlayers(2);
    state->startRound();
    state->getPlayer("player-0")->getHand().addCard(
        std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::ONE));
    state->getPlayer("player-1")->getHand().addCard(
        std::make_unique<core::Card>(core::Suit::WHOT, core::CardValue::TWENTY));
    std::string json = state->toJsonForPlayer("player-0");
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.contains("players"));
    ASSERT_EQ(j["players"].size(), 2u);
    EXPECT_TRUE(j["players"][0].contains("hand"));
    EXPECT_TRUE(j["players"][1]["hand"].contains("count"));
    EXPECT_EQ(j["players"][1]["hand"]["count"], 1);
}

TEST(TestGameState, CheckRoundEndCheckGameEnd) {
    auto state = makeGameStateWithPlayers(2);
    state->startRound();
    EXPECT_TRUE(state->checkRoundEnd());
    state->endRound();
    EXPECT_EQ(state->getPhase(), GamePhase::ROUND_ENDED);
    state->endGame();
    EXPECT_TRUE(state->checkGameEnd());
}

TEST(TestGameState, GetWinnerId) {
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    auto w = state->getWinnerId();
    ASSERT_TRUE(w.has_value());
    EXPECT_EQ(*w, "player-0");
}

TEST(TestGameState, FromJson_Malformed_Throws) {
    EXPECT_THROW(GameState::fromJson(""), nlohmann::json::parse_error);
}

} // namespace whot::game
