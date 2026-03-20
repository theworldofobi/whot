#include <gtest/gtest.h>
#include "Core/GameConstants.hpp"

namespace whot::core {

TEST(TestGameConstants, Constants) {
    EXPECT_EQ(STANDARD_DECK_SIZE, 54);
    EXPECT_EQ(WHOT_CARDS_COUNT, 5);
    EXPECT_EQ(DEFAULT_STARTING_CARDS, 6);
    EXPECT_EQ(NIGERIAN_MIN_CARDS, 3);
    EXPECT_EQ(NIGERIAN_MAX_CARDS, 6);
    EXPECT_EQ(DEFAULT_TURN_TIME_SECONDS, 10);
    EXPECT_EQ(ELIMINATION_SCORE, 100);
}

TEST(TestGameConstants, SuitToString) {
    EXPECT_EQ(suitToString(Suit::CIRCLE), "CIRCLE");
    EXPECT_EQ(suitToString(Suit::TRIANGLE), "TRIANGLE");
    EXPECT_EQ(suitToString(Suit::CROSS), "CROSS");
    EXPECT_EQ(suitToString(Suit::BLOCK), "BLOCK");
    EXPECT_EQ(suitToString(Suit::STAR), "STAR");
    EXPECT_EQ(suitToString(Suit::WHOT), "WHOT");
}

TEST(TestGameConstants, StringToSuit_Valid) {
    EXPECT_EQ(stringToSuit("CIRCLE"), Suit::CIRCLE);
    EXPECT_EQ(stringToSuit("TRIANGLE"), Suit::TRIANGLE);
    EXPECT_EQ(stringToSuit("CROSS"), Suit::CROSS);
    EXPECT_EQ(stringToSuit("BLOCK"), Suit::BLOCK);
    EXPECT_EQ(stringToSuit("STAR"), Suit::STAR);
    EXPECT_EQ(stringToSuit("WHOT"), Suit::WHOT);
}

TEST(TestGameConstants, StringToSuit_Invalid_FallbackToCircle) {
    EXPECT_EQ(stringToSuit(""), Suit::CIRCLE);
    EXPECT_EQ(stringToSuit("INVALID"), Suit::CIRCLE);
    EXPECT_EQ(stringToSuit("SQUARE"), Suit::CIRCLE);
}

TEST(TestGameConstants, CardValueToString) {
    EXPECT_EQ(cardValueToString(CardValue::ONE), "1");
    EXPECT_EQ(cardValueToString(CardValue::TWO), "2");
    EXPECT_EQ(cardValueToString(CardValue::FIVE), "5");
    EXPECT_EQ(cardValueToString(CardValue::SEVEN), "7");
    EXPECT_EQ(cardValueToString(CardValue::EIGHT), "8");
    EXPECT_EQ(cardValueToString(CardValue::TEN), "10");
    EXPECT_EQ(cardValueToString(CardValue::ELEVEN), "11");
    EXPECT_EQ(cardValueToString(CardValue::TWELVE), "12");
    EXPECT_EQ(cardValueToString(CardValue::THIRTEEN), "13");
    EXPECT_EQ(cardValueToString(CardValue::FOURTEEN), "14");
    EXPECT_EQ(cardValueToString(CardValue::TWENTY), "20");
}

TEST(TestGameConstants, StringToCardValue_Valid) {
    EXPECT_EQ(stringToCardValue("ONE"), CardValue::ONE);
    EXPECT_EQ(stringToCardValue("FIVE"), CardValue::FIVE);
    EXPECT_EQ(stringToCardValue("TWENTY"), CardValue::TWENTY);
}

TEST(TestGameConstants, StringToCardValue_Invalid_FallbackToOne) {
    EXPECT_EQ(stringToCardValue(""), CardValue::ONE);
    EXPECT_EQ(stringToCardValue("NINETY"), CardValue::ONE);
    EXPECT_EQ(stringToCardValue("INVALID"), CardValue::ONE);
}

TEST(TestGameConstants, StringToCardValue_Numeric) {
    EXPECT_EQ(stringToCardValue("1"), CardValue::ONE);
    EXPECT_EQ(stringToCardValue("20"), CardValue::TWENTY);
}

} // namespace whot::core
