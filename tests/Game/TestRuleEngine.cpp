#include <gtest/gtest.h>
#include "Game/RuleEngine.hpp"
#include "Game/GameState.hpp"
#include "TestHelpers.hpp"
#include "Core/Card.hpp"

namespace whot::game {

using namespace whot::test;

TEST(TestRuleEngine, CanPlayCard_WithCallCard) {
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    state->setCallCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::FIVE));
    core::Player* p = state->getCurrentPlayer();
    p->getHand().addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::THREE));
    RuleEngine re;
    core::Card card(core::Suit::CIRCLE, core::CardValue::THREE);
    EXPECT_TRUE(re.canPlayCard(*state, *p, card));
}

TEST(TestRuleEngine, MustDrawCard_NoPlayable) {
    auto state = makeGameStateWithPlayers(1);
    state->startRound();
    state->setCallCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::FIVE));
    core::Player* p = state->getCurrentPlayer();
    p->getHand().addCard(std::make_unique<core::Card>(core::Suit::TRIANGLE, core::CardValue::SEVEN));
    RuleEngine re;
    EXPECT_TRUE(re.mustDrawCard(*state, *p));
}

TEST(TestRuleEngine, CalculateDrawCount) {
    auto state = makeGameStateWithPlayers(1);
    state->setActivePickCount(4);
    RuleEngine re;
    core::Player p("p", "P", core::PlayerType::HUMAN);
    int count = re.calculateDrawCount(*state, p);
    EXPECT_GE(count, 1);
}

TEST(TestRuleEngine, RequiresLastCardDeclaration) {
    RuleEngine re;
    core::Player p("p", "P", core::PlayerType::HUMAN);
    p.getHand().addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::ONE));
    p.getHand().addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::TWO));
    EXPECT_TRUE(re.requiresLastCardDeclaration(p));
}

TEST(TestRuleEngine, RequiresCheckUpDeclaration) {
    RuleEngine re;
    core::Player p("p", "P", core::PlayerType::HUMAN);
    p.getHand().addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::ONE));
    EXPECT_TRUE(re.requiresCheckUpDeclaration(p));
}

TEST(TestRuleEngine, GetDeclarationPenalty) {
    RuleEngine re;
    EXPECT_GE(re.getDeclarationPenalty(), 0);
}

TEST(TestRuleEngine, CalculateRoundScore) {
    RuleEngine re;
    core::Player p("p", "P", core::PlayerType::HUMAN);
    p.getHand().addCard(std::make_unique<core::Card>(core::Suit::STAR, core::CardValue::FIVE));
    int score = re.calculateRoundScore(p);
    EXPECT_GE(score, 0);
}

TEST(TestRuleEngine, IsPlayerEliminated) {
    RuleEngine re;
    core::Player p("p", "P", core::PlayerType::HUMAN);
    p.addToScore(100);
    EXPECT_TRUE(re.isPlayerEliminated(p));
}

TEST(TestRuleEngine, GetTurnTimeLimit) {
    RuleEngine re;
    EXPECT_GE(re.getTurnTimeLimit(), 0);
}

} // namespace whot::game
