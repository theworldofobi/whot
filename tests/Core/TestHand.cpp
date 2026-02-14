#include <gtest/gtest.h>
#include "Core/Hand.hpp"
#include "Core/Card.hpp"
#include "TestHelpers.hpp"

namespace whot::core {

using namespace whot::test;

TEST(TestHand, EmptyHand) {
    Hand h;
    EXPECT_TRUE(h.isEmpty());
    EXPECT_EQ(h.size(), 0u);
}

TEST(TestHand, AddCard) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    EXPECT_EQ(h.size(), 1u);
    EXPECT_FALSE(h.isEmpty());
}

TEST(TestHand, AddCardNull_NoOp) {
    Hand h;
    h.addCard(nullptr);
    EXPECT_EQ(h.size(), 0u);
}

TEST(TestHand, PlayCard_IndexOutOfRange_ReturnsNull) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    EXPECT_EQ(h.playCard(1), nullptr);
    EXPECT_EQ(h.playCard(100), nullptr);
    EXPECT_EQ(h.size(), 1u);
}

TEST(TestHand, PlayCard_ByIndex) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    auto c = h.playCard(0);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->getSuit(), Suit::CIRCLE);
    EXPECT_EQ(h.size(), 0u);
}

TEST(TestHand, PlayCard_ByCard_NotInHand_ReturnsNull) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    Card other(Suit::TRIANGLE, CardValue::TWO);
    EXPECT_EQ(h.playCard(other), nullptr);
    EXPECT_EQ(h.size(), 1u);
}

TEST(TestHand, PlayCard_ByCard_InHand) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    Card target(Suit::CIRCLE, CardValue::ONE);
    auto c = h.playCard(target);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->getSuit(), Suit::CIRCLE);
    EXPECT_EQ(h.size(), 0u);
}

TEST(TestHand, GetCard_OutOfRange_Throws) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    EXPECT_THROW(h.getCard(1), std::out_of_range);
    EXPECT_THROW(h.getCard(100), std::out_of_range);
}

TEST(TestHand, GetCard_ValidIndex) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::BLOCK, CardValue::TEN));
    const Card& c = h.getCard(0);
    EXPECT_EQ(c.getSuit(), Suit::BLOCK);
    EXPECT_EQ(c.getValue(), CardValue::TEN);
}

TEST(TestHand, GetPlayableCardIndices_None) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    Card call(Suit::TRIANGLE, CardValue::SEVEN);
    auto indices = h.getPlayableCardIndices(call);
    EXPECT_TRUE(indices.empty());
}

TEST(TestHand, GetPlayableCardIndices_MatchSuit) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    h.addCard(std::make_unique<Card>(Suit::TRIANGLE, CardValue::TWO));
    Card call(Suit::TRIANGLE, CardValue::SEVEN);
    auto indices = h.getPlayableCardIndices(call);
    EXPECT_EQ(indices.size(), 1u);
    EXPECT_EQ(indices[0], 1u);
}

TEST(TestHand, HasPlayableCard) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::FIVE));
    Card call(Suit::CIRCLE, CardValue::THREE);
    EXPECT_TRUE(h.hasPlayableCard(call));
    Card call2(Suit::TRIANGLE, CardValue::EIGHT);
    EXPECT_FALSE(h.hasPlayableCard(call2));
}

TEST(TestHand, HasCard) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::FIVE));
    Card yes(Suit::CIRCLE, CardValue::FIVE);
    Card no(Suit::CIRCLE, CardValue::SEVEN);
    EXPECT_TRUE(h.hasCard(yes));
    EXPECT_FALSE(h.hasCard(no));
}

TEST(TestHand, CalculateTotalScore_Empty) {
    Hand h;
    EXPECT_EQ(h.calculateTotalScore(), 0);
}

TEST(TestHand, CalculateTotalScore_WithCards) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::FIVE));
    h.addCard(std::make_unique<Card>(Suit::STAR, CardValue::FIVE));
    int score = h.calculateTotalScore();
    EXPECT_GE(score, 5 + 10);
}

TEST(TestHand, ToJsonFromJson_RoundTrip) {
    auto hand = makeHandWithCards({{Suit::CIRCLE, CardValue::ONE}, {Suit::WHOT, CardValue::TWENTY}});
    std::string json = hand->toJson();
    Hand restored = Hand::fromJson(json);
    EXPECT_EQ(restored.size(), 2u);
}

TEST(TestHand, GetCardsBySuit) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::TWO));
    h.addCard(std::make_unique<Card>(Suit::TRIANGLE, CardValue::ONE));
    auto circle = h.getCardsBySuit(Suit::CIRCLE);
    EXPECT_EQ(circle.size(), 2u);
}

TEST(TestHand, GetWhotCards) {
    Hand h;
    h.addCard(std::make_unique<Card>(Suit::WHOT, CardValue::TWENTY));
    h.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::ONE));
    auto whots = h.getWhotCards();
    EXPECT_EQ(whots.size(), 1u);
}

} // namespace whot::core
