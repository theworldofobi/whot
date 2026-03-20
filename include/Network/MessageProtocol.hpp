#ifndef WHOT_NETWORK_MESSAGE_PROTOCOL_HPP
#define WHOT_NETWORK_MESSAGE_PROTOCOL_HPP

#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <map>
#include "Core/Card.hpp"

namespace whot::network {

enum class MessageType : uint16_t {
    // Client -> Server
    JOIN_GAME = 100,
    LEAVE_GAME = 101,
    START_GAME = 109,
    PLAY_CARD = 102,
    DRAW_CARD = 103,
    DECLARE_LAST_CARD = 104,
    DECLARE_CHECK_UP = 105,
    CHOOSE_SUIT = 106,
    CHAT_MESSAGE = 107,
    READY_UP = 108,
    
    // Server -> Client
    GAME_STATE_UPDATE = 200,
    PLAYER_JOINED = 201,
    PLAYER_LEFT = 202,
    TURN_STARTED = 203,
    CARD_PLAYED = 204,
    CARD_DRAWN = 205,
    ROUND_ENDED = 206,
    GAME_ENDED = 207,
    ERROR = 208,
    CHAT_BROADCAST = 209,
    
    // Bidirectional
    PING = 300,
    PONG = 301
};

struct Message {
    MessageType type;
    std::string playerId;
    std::string gameId;
    std::string payload;  // JSON payload
    uint64_t timestamp;
    
    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

// Specific message payloads
struct JoinGamePayload {
    std::string playerName;
    std::string gameId;
    std::optional<std::string> gameCode;  // Join by code (Kahoot-style)
    std::optional<std::string> password;
    
    std::string toJson() const;
    static JoinGamePayload fromJson(const std::string& json);
};

struct PlayCardPayload {
    size_t cardIndex;
    std::optional<core::Suit> chosenSuit;  // For Whot cards
    std::optional<bool> reverseDirection;
    std::vector<size_t> additionalCards;  // For straights/double-decking
    
    std::string toJson() const;
    static PlayCardPayload fromJson(const std::string& json);
};

struct GameStateUpdatePayload {
    std::string gameStateJson;
    std::vector<std::string> visibleTo;  // Player-specific state
    
    std::string toJson() const;
    static GameStateUpdatePayload fromJson(const std::string& json);
};

struct CardPlayedPayload {
    std::string playerId;
    std::string cardJson;  // May be hidden for opponents
    int cardsRemaining;
    bool saidLastCard;
    
    std::string toJson() const;
    static CardPlayedPayload fromJson(const std::string& json);
};

struct RoundEndedPayload {
    std::string winnerId;
    std::map<std::string, int> scores;
    std::vector<std::string> eliminatedPlayers;
    
    std::string toJson() const;
    static RoundEndedPayload fromJson(const std::string& json);
};

struct ErrorPayload {
    std::string errorCode;
    std::string message;
    std::optional<std::string> field;
    
    std::string toJson() const;
    static ErrorPayload fromJson(const std::string& json);
};

} // namespace whot::network

#endif // WHOT_NETWORK_MESSAGE_PROTOCOL_HPP
