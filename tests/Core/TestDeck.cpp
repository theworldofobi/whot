#include <gtest/gtest.h>
#include "Core/Deck.hpp"
#include "Core/Card.hpp"
#include "Core/GameConstants.hpp"
#include "TestHelpers.hpp"
#include <nlohmann/json.hpp>

namespace whot::core {

using namespace whot::test;

TEST(TestDeck, DefaultDeckSize) {
    Deck d;  // ctor already calls createDeck() once
    EXPECT_EQ(d.size(), STANDARD_DECK_SIZE);
}

TEST(TestDeck, InitializeStandardDeck) {
    Deck d;
    d.initializeStandardDeck();
    EXPECT_EQ(d.size(), STANDARD_DECK_SIZE);
}

TEST(TestDeck, TwoDecks) {
    Deck d(2);
    EXPECT_EQ(d.size(), 2u * STANDARD_DECK_SIZE);
}

TEST(TestDeck, DrawUntilEmpty) {
    Deck d;
    d.initializeStandardDeck();
    size_t count = d.size();
    for (size_t i = 0; i < count; ++i) {
        auto card = d.draw();
        ASSERT_NE(card, nullptr);
    }
    EXPECT_TRUE(d.isEmpty());
    EXPECT_EQ(d.draw(), nullptr);
}

TEST(TestDeck, AddCardNull_NoOp) {
    Deck d;
    d.clear();
    d.addCard(nullptr);
    EXPECT_EQ(d.size(), 0u);
}

TEST(TestDeck, PeekEmpty_ReturnsNull) {
    Deck d;
    d.clear();
    EXPECT_EQ(d.peek(), nullptr);
}

TEST(TestDeck, PeekNonEmpty) {
    Deck d;
    d.clear();
    d.addCard(std::make_unique<Card>(Suit::CIRCLE, CardValue::FIVE));
    const Card* p = d.peek();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->getSuit(), Suit::CIRCLE);
    EXPECT_EQ(p->getValue(), CardValue::FIVE);
}

TEST(TestDeck, ToJsonFromJson_EmptyDeck) {
    Deck d;
    d.clear();
    std::string json = d.toJson();
    Deck restored = Deck::fromJson(json);
    EXPECT_EQ(restored.size(), 0u);
}

TEST(TestDeck, ToJsonFromJson_SingleCard) {
    Deck d;
    d.clear();
    d.addCard(std::make_unique<Card>(Suit::STAR, CardValue::TWENTY));
    std::string json = d.toJson();
    Deck restored = Deck::fromJson(json);
    ASSERT_EQ(restored.size(), 1u);
    auto c = restored.draw();
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->getSuit(), Suit::STAR);
    EXPECT_EQ(c->getValue(), CardValue::TWENTY);
}

TEST(TestDeck, FromJson_Malformed_Throws) {
    EXPECT_THROW(Deck::fromJson("not json"), nlohmann::json::parse_error);
}

TEST(TestDeck, FromJson_EmptyArray) {
    Deck d = Deck::fromJson("[]");
    EXPECT_EQ(d.size(), 0u);
}

TEST(TestDeck, ReshuffleFromDiscardPile_Empty) {
    Deck d;
    d.clear();
    Card call(Suit::CIRCLE, CardValue::FIVE);
    std::vector<std::unique_ptr<Card>> pile;
    d.reshuffleFromDiscardPile(std::move(pile), call);
    EXPECT_EQ(d.size(), 0u);
}

TEST(TestDeck, ReshuffleFromDiscardPile_WithCallCard) {
    Deck d;
    d.clear();
    Card call(Suit::CIRCLE, CardValue::FIVE);
    std::vector<std::unique_ptr<Card>> pile;
    pile.push_back(std::make_unique<Card>(Suit::CIRCLE, CardValue::THREE));
    pile.push_back(std::make_unique<Card>(Suit::CIRCLE, CardValue::FIVE));
    d.reshuffleFromDiscardPile(std::move(pile), call);
    EXPECT_EQ(d.size(), 1u);
}

TEST(TestDeck, Reset) {
    Deck d;
    d.initializeStandardDeck();
    d.draw();
    d.reset();
    EXPECT_EQ(d.size(), STANDARD_DECK_SIZE);
}

TEST(TestDeck, Clear) {
    Deck d;
    d.initializeStandardDeck();
    d.clear();
    EXPECT_TRUE(d.isEmpty());
}

} // namespace whot::core
