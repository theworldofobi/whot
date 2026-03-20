#include <gtest/gtest.h>
#include "Rules/NigerianRules.hpp"
#include "Core/Card.hpp"
#include "Core/Player.hpp"

namespace whot::rules {

TEST(TestNigerianRules, RuleFlagsAndConstants) {
    NigerianRules r;
    EXPECT_GE(r.getDefaultStartingCards(), 3);
    EXPECT_LE(r.getDefaultStartingCards(), 6);
    EXPECT_EQ(r.getTurnTimeSeconds(), 10);
    EXPECT_EQ(r.getEliminationScore(), 100);
    EXPECT_TRUE(r.requiresLastCardDeclaration());
    EXPECT_TRUE(r.requiresCheckUpDeclaration());
    EXPECT_GE(r.getDeclarationPenalty(), 0);
    EXPECT_FALSE(r.getRuleName().empty());
}

TEST(TestNigerianRules, CanPlayCard_WhotOnAnything) {
    NigerianRules r;
    core::Card whot(core::Suit::WHOT, core::CardValue::TWENTY);
    core::Card call(core::Suit::CIRCLE, core::CardValue::FIVE);
    core::Player p("p", "P", core::PlayerType::HUMAN);
    EXPECT_TRUE(r.canPlayCard(whot, call, p, std::nullopt));
}

TEST(TestNigerianRules, CanPlayCard_DemandedSuit) {
    NigerianRules r;
    core::Card card(core::Suit::BLOCK, core::CardValue::THREE);
    core::Card call(core::Suit::CIRCLE, core::CardValue::FIVE);
    core::Player p("p", "P", core::PlayerType::HUMAN);
    EXPECT_FALSE(r.canPlayCard(card, call, p, core::Suit::CIRCLE));
    EXPECT_TRUE(r.canPlayCard(card, call, p, core::Suit::BLOCK));
}

TEST(TestNigerianRules, CanDefendAgainstAttack) {
    NigerianRules r;
    core::Card two(core::Suit::CIRCLE, core::CardValue::TWO);
    core::Card five(core::Suit::CIRCLE, core::CardValue::FIVE);
    EXPECT_TRUE(r.canDefendAgainstAttack(two, two));
    EXPECT_FALSE(r.canDefendAgainstAttack(two, five));
}

TEST(TestNigerianRules, ValidateDoubleDecking_Empty) {
    NigerianRules r;
    std::vector<const core::Card*> cards;
    core::Card call(core::Suit::CIRCLE, core::CardValue::FIVE);
    EXPECT_FALSE(r.validateDoubleDecking(cards, call));
}

TEST(TestNigerianRules, CalculateScore) {
    NigerianRules r;
    core::Hand hand;
    hand.addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::FIVE));
    EXPECT_EQ(r.calculateScore(hand), 5);
}

TEST(TestNigerianRules, Setters) {
    NigerianRules r;
    r.setStartingCards(4);
    EXPECT_EQ(r.getDefaultStartingCards(), 4);
    r.setAllowDoubleDecking(true);
    EXPECT_TRUE(r.allowDoubleDecking());
}

} // namespace whot::rules
