#include <gtest/gtest.h>
#include "Network/MessageProtocol.hpp"
#include "Core/GameConstants.hpp"

namespace whot::network {

TEST(TestMessageProtocol, Message_SerializeDeserialize_RoundTrip) {
    Message m;
    m.type = MessageType::JOIN_GAME;
    m.playerId = "p1";
    m.gameId = "g1";
    m.payload = "{\"playerName\":\"Alice\"}";
    m.timestamp = 12345;
    std::string data = m.serialize();
    Message restored = Message::deserialize(data);
    EXPECT_EQ(restored.type, MessageType::JOIN_GAME);
    EXPECT_EQ(restored.playerId, "p1");
    EXPECT_EQ(restored.gameId, "g1");
    EXPECT_EQ(restored.payload, m.payload);
    EXPECT_EQ(restored.timestamp, 12345u);
}

TEST(TestMessageProtocol, Message_Deserialize_EmptyString_DefaultsToError) {
    Message m = Message::deserialize("");
    EXPECT_EQ(m.type, MessageType::ERROR);
}

TEST(TestMessageProtocol, Message_Deserialize_Malformed_DefaultsToError) {
    Message m = Message::deserialize("not json");
    EXPECT_EQ(m.type, MessageType::ERROR);
}

TEST(TestMessageProtocol, Message_Deserialize_MissingType_DefaultsToError) {
    Message m = Message::deserialize("{}");
    EXPECT_EQ(m.type, MessageType::ERROR);
}

TEST(TestMessageProtocol, JoinGamePayload_RoundTrip) {
    JoinGamePayload p;
    p.playerName = "Bob";
    p.gameId = "game-1";
    p.password = "secret";
    std::string json = p.toJson();
    JoinGamePayload restored = JoinGamePayload::fromJson(json);
    EXPECT_EQ(restored.playerName, "Bob");
    EXPECT_EQ(restored.gameId, "game-1");
    ASSERT_TRUE(restored.password.has_value());
    EXPECT_EQ(*restored.password, "secret");
}

TEST(TestMessageProtocol, JoinGamePayload_FromJson_MissingFields) {
    JoinGamePayload p = JoinGamePayload::fromJson("{}");
    EXPECT_TRUE(p.playerName.empty());
    EXPECT_TRUE(p.gameId.empty());
    EXPECT_FALSE(p.password.has_value());
}

TEST(TestMessageProtocol, PlayCardPayload_FromJson) {
    PlayCardPayload p = PlayCardPayload::fromJson("{\"cardIndex\": 2, \"chosenSuit\": \"BLOCK\"}");
    EXPECT_EQ(p.cardIndex, 2u);
    ASSERT_TRUE(p.chosenSuit.has_value());
    EXPECT_EQ(*p.chosenSuit, core::Suit::BLOCK);
}

TEST(TestMessageProtocol, PlayCardPayload_FromJson_InvalidChosenSuit_UsesFallback) {
    PlayCardPayload p = PlayCardPayload::fromJson("{\"chosenSuit\": \"INVALID\"}");
    ASSERT_TRUE(p.chosenSuit.has_value());
    EXPECT_EQ(*p.chosenSuit, core::Suit::CIRCLE);
}

TEST(TestMessageProtocol, PlayCardPayload_FromJson_AdditionalCardsNonArray) {
    PlayCardPayload p = PlayCardPayload::fromJson("{\"additionalCards\": \"notarray\"}");
    EXPECT_TRUE(p.additionalCards.empty());
}

TEST(TestMessageProtocol, ErrorPayload_RoundTrip) {
    ErrorPayload p;
    p.errorCode = "E001";
    p.message = "Something failed";
    p.field = "gameId";
    std::string json = p.toJson();
    ErrorPayload restored = ErrorPayload::fromJson(json);
    EXPECT_EQ(restored.errorCode, "E001");
    EXPECT_EQ(restored.message, "Something failed");
    ASSERT_TRUE(restored.field.has_value());
    EXPECT_EQ(*restored.field, "gameId");
}

TEST(TestMessageProtocol, ErrorPayload_FromJson_Empty) {
    ErrorPayload p = ErrorPayload::fromJson("{}");
    EXPECT_TRUE(p.errorCode.empty());
    EXPECT_TRUE(p.message.empty());
}

TEST(TestMessageProtocol, GameStateUpdatePayload_FromJson) {
    GameStateUpdatePayload p = GameStateUpdatePayload::fromJson(
        "{\"gameStateJson\": \"{}\", \"visibleTo\": [\"p1\", \"p2\"]}");
    EXPECT_EQ(p.gameStateJson, "{}");
    EXPECT_EQ(p.visibleTo.size(), 2u);
}

} // namespace whot::network
