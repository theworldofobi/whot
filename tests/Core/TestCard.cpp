#include <gtest/gtest.h>
#include "Core/Card.hpp"
#include "Core/GameConstants.hpp"
#include "TestHelpers.hpp"
#include <nlohmann/json.hpp>

namespace whot::core {

using namespace whot::test;

TEST(TestCard, AllSuitsAndValues) {
    for (int s = 0; s <= static_cast<int>(Suit::WHOT); ++s) {
        Suit suit = static_cast<Suit>(s);
        Card c(suit, CardValue::ONE);
        EXPECT_EQ(c.getSuit(), suit);
        EXPECT_EQ(c.getValue(), CardValue::ONE);
    }
}

TEST(TestCard, GetNumericValue) {
    Card c(Suit::CIRCLE, CardValue::FIVE);
    EXPECT_EQ(c.getNumericValue(), 5);
    Card c20(Suit::WHOT, CardValue::TWENTY);
    EXPECT_EQ(c20.getNumericValue(), 20);
}

TEST(TestCard, GetScoreValue_StarDoubled) {
    Card star(Suit::STAR, CardValue::FIVE);
    EXPECT_TRUE(star.isStarCard());
    EXPECT_EQ(star.getOuterStarValue(), 5);
    EXPECT_EQ(star.getInnerStarValue(), 10);
    EXPECT_EQ(star.getScoreValue(), 10);
}

TEST(TestCard, GetScoreValue_NonStar) {
    Card c(Suit::CIRCLE, CardValue::SEVEN);
    EXPECT_EQ(c.getScoreValue(), 7);
}

TEST(TestCard, CanPlayOn_WhotOnAnything) {
    Card whot(Suit::WHOT, CardValue::TWENTY);
    Card call(Suit::CIRCLE, CardValue::FIVE);
    EXPECT_TRUE(whot.canPlayOn(call));
}

TEST(TestCard, CanPlayOn_SameSuit) {
    Card c(Suit::TRIANGLE, CardValue::THREE);
    Card call(Suit::TRIANGLE, CardValue::SEVEN);
    EXPECT_TRUE(c.canPlayOn(call));
}

TEST(TestCard, CanPlayOn_SameValue) {
    Card c(Suit::CROSS, CardValue::FIVE);
    Card call(Suit::BLOCK, CardValue::FIVE);
    EXPECT_TRUE(c.canPlayOn(call));
}

TEST(TestCard, CanPlayOn_NoMatch) {
    Card c(Suit::CIRCLE, CardValue::ONE);
    Card call(Suit::TRIANGLE, CardValue::SEVEN);
    EXPECT_FALSE(c.canPlayOn(call));
}

TEST(TestCard, OperatorEqNe) {
    Card a(Suit::BLOCK, CardValue::TEN);
    Card b(Suit::BLOCK, CardValue::TEN);
    Card c(Suit::STAR, CardValue::TEN);
    EXPECT_EQ(a, b);
    EXPECT_FALSE(a != b);
    EXPECT_NE(a, c);
    EXPECT_FALSE(a == c);
}

TEST(TestCard, ToJsonRoundTrip) {
    Card c(Suit::CIRCLE, CardValue::FIVE);
    std::string json = c.toJson();
    auto restored = Card::fromJson(json);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(*restored, c);
}

TEST(TestCard, FromJson_EmptyString_Throws) {
    EXPECT_THROW(Card::fromJson(""), nlohmann::json::parse_error);
}

TEST(TestCard, FromJson_InvalidJson_Throws) {
    EXPECT_THROW(Card::fromJson("not json"), nlohmann::json::parse_error);
    EXPECT_THROW(Card::fromJson("{ invalid }"), nlohmann::json::parse_error);
}

TEST(TestCard, FromJson_EmptyObject_Throws) {
    EXPECT_THROW(Card::fromJson("{}"), nlohmann::json::exception);
}

TEST(TestCard, FromJson_MissingSuit_Throws) {
    EXPECT_THROW(Card::fromJson("{\"value\": \"FIVE\"}"), nlohmann::json::exception);
}

TEST(TestCard, FromJson_MissingValue_Throws) {
    EXPECT_THROW(Card::fromJson("{\"suit\": \"CIRCLE\"}"), nlohmann::json::exception);
}

TEST(TestCard, FromJson_InvalidSuitString_UsesFallback) {
    auto card = Card::fromJson("{\"suit\": \"INVALID\", \"value\": \"FIVE\"}");
    ASSERT_NE(card, nullptr);
    EXPECT_EQ(card->getSuit(), Suit::CIRCLE);
    EXPECT_EQ(card->getValue(), CardValue::FIVE);
}

TEST(TestCard, FromJson_InvalidValueString_UsesFallback) {
    auto card = Card::fromJson("{\"suit\": \"CIRCLE\", \"value\": \"NINETY\"}");
    ASSERT_NE(card, nullptr);
    EXPECT_EQ(card->getValue(), CardValue::ONE);
}

TEST(TestCard, FromJson_NonStringSuit_Throws) {
    EXPECT_THROW(Card::fromJson("{\"suit\": 1, \"value\": \"FIVE\"}"), nlohmann::json::exception);
}

TEST(TestCard, FromJson_NonStringValue_Throws) {
    EXPECT_THROW(Card::fromJson("{\"suit\": \"CIRCLE\", \"value\": 5}"), nlohmann::json::exception);
}

TEST(TestCard, SpecialAbility) {
    Card holdOn(Suit::CIRCLE, CardValue::ONE);
    EXPECT_EQ(holdOn.getSpecialAbility(), SpecialAbility::HOLD_ON);
    Card pickTwo(Suit::CIRCLE, CardValue::TWO);
    EXPECT_EQ(pickTwo.getSpecialAbility(), SpecialAbility::PICK_TWO);
    Card general(Suit::CIRCLE, CardValue::FOURTEEN);
    EXPECT_EQ(general.getSpecialAbility(), SpecialAbility::GENERAL_MARKET);
    Card whot(Suit::WHOT, CardValue::TWENTY);
    EXPECT_TRUE(whot.isWhotCard());
    EXPECT_EQ(whot.getSpecialAbility(), SpecialAbility::WHOT_CARD);
}

TEST(TestCard, ToString) {
    Card c(Suit::BLOCK, CardValue::TEN);
    std::string s = c.toString();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("10"), std::string::npos);
}

} // namespace whot::core
